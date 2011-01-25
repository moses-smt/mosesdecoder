//
//  View2.m
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "View1.h"
#import "View2.h"
extern "C" {
#import "minzipwrapper.h"
}
#import "CFunctions.h"


@implementation View2
@synthesize folderNames;
@synthesize busyIndicator;

// The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
/*
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization.
    }
    return self;
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	
	timer = [NSTimer scheduledTimerWithTimeInterval:2.0
																					 target:self
																				 selector:@selector(targetMethod:)
																				 userInfo:nil
																					repeats:NO];
	
}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations.
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc {
	[self.folderNames dealloc];
	[timer dealloc];
    [super dealloc];
}

- (IBAction) downloadButtonWasTouched:(id)sender
{
	[self downloadNow];
	txtURL.enabled = NO;
	txtURL.enabled = YES;
}

- (IBAction) downloadNow
{
	[busyIndicator startAnimating];

	NSString *urlStr = txtURL.text;
	NSLog(urlStr);
	
	NSURL *url = [NSURL URLWithString:urlStr];
	NSData *data = [NSData dataWithContentsOfURL:url];
	
	if (data != nil) 
	{
		NSLog(@"\nis not nil");
		NSString *fileName = [url lastPathComponent];
		
		[data writeToFile:fileName atomically:YES];
		[self InitView];
	}	
	else {
		NSLog(@"nothing download");
		alertView = [ [UIAlertView alloc]
								 initWithTitle:@"Error"
								 message:@"Couldn't download model" 
								 delegate:self
								 cancelButtonTitle:@"Close"
								 otherButtonTitles:nil ];
		// show alert
		[alertView show];
		//	[alertView release];
	}

	
	[busyIndicator stopAnimating];
}

// Customize the number of rows in the table view.
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	NSInteger num = self.folderNames.count;
	return num;
}

// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	
	static NSString *CellIdentifier = @"Cell";
	
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
	if (cell == nil) {
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
	}
	
	NSInteger row = [indexPath row];
	NSString *txt = [self.folderNames objectAtIndex:row];
	// Configure the cell.
	cell.textLabel.text = txt;
	
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	NSLog(@"load");

	[busyIndicator startAnimating];

	//Get the dictionary of the selected data source.
	NSInteger row = indexPath.row;	
	NSString *modelDir = [self.folderNames objectAtIndex:row];
	
	NSLog(modelDir);
	
		
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectoryNS = [paths objectAtIndex:0];      
	
	NSString *modelPathNS = [documentsDirectoryNS stringByAppendingPathComponent:@"/"];
	modelPathNS = [modelPathNS stringByAppendingPathComponent:modelDir];
	const char *modelPath = [modelPathNS cStringUsingEncoding: NSASCIIStringEncoding  ];
	
	NSString *iniPathNS = [modelPathNS stringByAppendingPathComponent:@"/moses.ini"]; 
	const char *iniPath = [iniPathNS cStringUsingEncoding: NSASCIIStringEncoding  ];
	NSLog(iniPathNS);
	
	char source[1000];
	char target[1000];
	char description[1000];
	
	
	int ret = LoadModel(modelPath, iniPath, source, target, description);
	
	if (ret)
	{
		NSLog(@"oh dear");
		// Create a suitable alert view
		alertView = [ [UIAlertView alloc]
								 initWithTitle:@"Error"
								 message:@"Can't load model" 
								 delegate:self
								 cancelButtonTitle:@"Close"
								 otherButtonTitles:nil ];
		// show alert
		[alertView show];
		//	[alertView release];

		View1 *viewTranslate = [self.tabBarController.viewControllers objectAtIndex:0];		
		viewTranslate.view.userInteractionEnabled = NO;

	}
	else {
		NSLog(@"Loaded");
		
		// persistant storage
		NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];		
		[prefs setValue:modelDir forKey: @"currentModel"];
		[prefs synchronize];
		
		NSLog(modelDir);
		
		View1 *viewTranslate = [self.tabBarController.viewControllers objectAtIndex:0];		
		viewTranslate.view.userInteractionEnabled = YES;
		self.tabBarController.selectedIndex = 0;
	
	}
	
	[busyIndicator stopAnimating];
	
}


- (void) targetMethod: (NSTimer*) callingTimer
{
	[self InitView];
}
	 
 - (void) InitView
 {
	NSLog(@"loadme");
	[busyIndicator startAnimating];
	
	//1st loop. unzip any files transferred by itunes
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *publicDocumentsDir = [paths objectAtIndex:0];   
	
	NSError *error;
	NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:publicDocumentsDir error:&error];
	
	for (NSString *file in files) 
	{
		NSString *fullPath = [publicDocumentsDir stringByAppendingPathComponent:@"/"]; 
		fullPath = [fullPath stringByAppendingPathComponent:file]; 
		NSLog(fullPath);
		
		if ([fullPath.pathExtension compare:@"zip" options:NSCaseInsensitiveSearch] == NSOrderedSame) 
		{        
			NSLog(@"unzipping");	
			
			const char *fileCStr = [fullPath cStringUsingEncoding: NSASCIIStringEncoding  ];
			const char *docPathCtr = [publicDocumentsDir cStringUsingEncoding: NSASCIIStringEncoding  ];
			
			Unzip(fileCStr, docPathCtr);
			remove(fileCStr);			
		}
	}
	
	// 2nd loop. List folder names
	self.folderNames = [[NSMutableArray alloc] init];
	
	files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:publicDocumentsDir error:&error];
	if (files == nil) {
		NSLog(@"Error reading contents of documents directory: %@", [error localizedDescription]);
		//return retval;
	}
	
	NSString *modelList = @"";
	
	for (NSString *file in files) 
	{
		NSString *fullPath = [publicDocumentsDir stringByAppendingPathComponent:@"/"]; 
		fullPath = [fullPath stringByAppendingPathComponent:file]; 
		fullPath = [fullPath stringByAppendingPathComponent:@"/moses.ini"]; 
		NSLog(fullPath);
		
		BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:fullPath];
		if (fileExists)
		{
			modelList = [modelList stringByAppendingPathComponent:@" "]; 
			modelList = [modelList stringByAppendingPathComponent:file]; 				
			
			[self.folderNames addObject:file];
			
		}
		
	}
	
	[tableView reloadData];
	[busyIndicator stopAnimating];
	
}


@end
