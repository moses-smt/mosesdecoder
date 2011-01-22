//
//  View2.m
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "View2.h"
extern "C" {
#import "minzipwrapper.h"
}
#import "CFunctions.h"


@implementation View2

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
	NSMutableArray *retval = [NSMutableArray array];
	
	
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *publicDocumentsDir = [paths objectAtIndex:0];   
	
	NSError *error;
	NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:publicDocumentsDir error:&error];
	if (files == nil) {
		NSLog(@"Error reading contents of documents directory: %@", [error localizedDescription]);
		//return retval;
	}
	
	NSString *modelList = @"";
	
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
		else 
		{
			NSLog(fullPath);
			fullPath = [fullPath stringByAppendingPathComponent:@"/moses.ini"]; 
			NSLog(fullPath);

			BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:fullPath];
			if (fileExists)
			{
				modelList = [modelList stringByAppendingPathComponent:@" "]; 
				modelList = [modelList stringByAppendingPathComponent:file]; 				
			}
		}
		
	}
	
	txtList.text = modelList;
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
    [super dealloc];
}

- (IBAction) loadButtonWasTouched
{
	NSLog(@"load");

	NSString *modelDir = txtLoad.text;

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
	}
	else {
		NSLog(@"Loaded");
		
		// persistant storage
		NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];		
		[prefs setValue:modelDir forKey: @"currentModel"];
		[prefs synchronize];
		
		NSLog(modelDir);
		
	}
	
}

- (IBAction) downloadButtonWasTouched
{
	NSString *url = txtURL.text;
	NSLog(url);
	
	
}

@end
