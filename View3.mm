//
//  View3.mm
//  moses
//
//  Created by Hieu Hoang on 22/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "View3.h"
#import "View1.h"
extern "C" {
#import "minzipwrapper.h"
}
#import "CFunctions.h"


@implementation View3
@synthesize btDownload;
@synthesize btRefresh;
@synthesize txtURL;
@synthesize busyIndicator; 
@synthesize folderNames;

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

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}
*/

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

- (void)viewDidAppear:(BOOL)animated {
	[self RefreshTableView];
}

- (void)dealloc {
		[self.folderNames dealloc];
		[timer dealloc];
  
    [super dealloc];
}

- (IBAction) downloadButtonWasTouched:(id)sender
{
	[busyIndicator startAnimating];
	
	[self downloadZipFile	];
	/*
	timer = [NSTimer scheduledTimerWithTimeInterval:1.0
																					 target:self
																				 selector:@selector(downloadZipFile:)
																				 userInfo:nil
																					repeats:NO];
	 */
	 [busyIndicator stopAnimating];	
}

- (void) downloadZipFile 
{
		
	NSString *urlStr = self.txtURL.text;
	NSLog(urlStr);
	
	NSURL *url = [NSURL URLWithString:urlStr];
	NSData *data = [NSData dataWithContentsOfURL:url];
	
	if (data != nil) 
	{
		NSLog(@"\nis not nil");
		NSString *fileName = [url lastPathComponent];
		
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *publicDocumentsDir = [paths objectAtIndex:0];   
		fileName = [ [publicDocumentsDir stringByAppendingPathComponent:@"/"] stringByAppendingPathComponent:fileName]; 

		[data writeToFile:fileName atomically:YES];
		[self RefreshTableView];
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
		
}

- (void) RefreshTableView
{
	NSLog(@"loadme");
	[busyIndicator startAnimating];
	
	//1st loop. unzip any files transferred by itunes
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *publicDocumentsDir = [paths objectAtIndex:0];   
	
	NSError *error;
	NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:publicDocumentsDir error:&error];
	
	// 2nd loop. List folder names
	self.folderNames = [[NSMutableArray alloc] init];

	for (NSString *file in files) 
	{
		NSString *fullPath = [publicDocumentsDir stringByAppendingPathComponent:@"/"]; 
		fullPath = [fullPath stringByAppendingPathComponent:file]; 
		NSLog(fullPath);
		
		if ([fullPath.pathExtension compare:@"zip" options:NSCaseInsensitiveSearch] == NSOrderedSame) 
		{        
			NSLog(@"zip file");	

			[self.folderNames addObject:file];
		}
	}
	
	
	[tableView reloadData];
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
	NSLog(@"unzipping");	
	
	[busyIndicator startAnimating];
	
	selectedIndexPath = indexPath;
	
	timer = [NSTimer scheduledTimerWithTimeInterval:1.0
																					 target:self
																				 selector:@selector(unzipFile)
																				 userInfo:nil
																					repeats:NO];
	
	[busyIndicator stopAnimating];
	
	self.tabBarController.selectedIndex = 1;

}

- (void) unzipFile
{
	//Get the dictionary of the selected data source.
	NSInteger row = selectedIndexPath.row;	
	NSString *zipFile = [self.folderNames objectAtIndex:row];
	
	NSLog(zipFile);
	
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectoryNS = [paths objectAtIndex:0];      
	
	NSString *fullPathNS = [documentsDirectoryNS stringByAppendingPathComponent:@"/"];
	fullPathNS = [fullPathNS stringByAppendingPathComponent:zipFile];
	
	const char *fullPath = [fullPathNS cStringUsingEncoding: NSASCIIStringEncoding  ];
	const char *docPathCtr = [documentsDirectoryNS cStringUsingEncoding: NSASCIIStringEncoding  ];
	
	Unzip(fullPath, docPathCtr);
	remove(fullPath);				
}

- (IBAction) refreshButtonWasTouched:(id)sender
{
	[self RefreshTableView];
}

@end
