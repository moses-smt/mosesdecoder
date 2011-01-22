//
//  View1.m
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "View1.h"
#import "CFunctions.h"

@implementation View1
@synthesize tab;

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

	// persistant storage
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	
	NSString *currModel = [prefs stringForKey:@"currentModel"];
	NSLog(currModel);
	
	// load model
	if (currModel != nil)
	{
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *documentsDirectoryNS = [paths objectAtIndex:0];      
		
		NSString *modelPathNS = [documentsDirectoryNS stringByAppendingPathComponent:@"/"];
		modelPathNS = [modelPathNS stringByAppendingPathComponent:currModel];
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
		}
		
	}
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

- (IBAction) translateButtonWasTouched
{
	[txtSource endEditing:YES];
	[self translateNow];
}

- (IBAction) translateNow
{
	NSLog(@"translate");
	
	NSString *sourceString = txtSource.text;
	NSLog(sourceString);
	
	sourceString = [sourceString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
	if ([sourceString length] > 0)
	{
		const char *cstring = [sourceString UTF8String];
		char* result = (char *) malloc(sizeof(char)*1000);
		TranslateSentence(cstring,result);
		
		NSString *targetString = [[NSString alloc] initWithUTF8String: result];
		NSLog(targetString);
		
		free(result);
		
		txTarget.text = targetString;
		
		[targetString release];
	}
}

- (void) alertView:(UIAlertView *)alertView
didDismissWithButtonIndex:(NSInteger) buttonIndex
{
	NSLog(@"button=%i", buttonIndex);
	
	self.tabBarController.selectedIndex = 1;
	self.view.userInteractionEnabled = NO;
}

@end
