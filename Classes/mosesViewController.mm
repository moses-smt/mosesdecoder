//
//  mosesViewController.m
//  moses
//
//  Created by Hieu Hoang on 19/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "CFunctions.h"
#import "mosesViewController.h"

@implementation mosesViewController

@synthesize doNotPressButton;
@synthesize sourceText;
@synthesize targetText;

/*
// The designated initializer. Override to perform setup that is required before the view is loaded.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}
*/

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];

	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectoryNS = [paths objectAtIndex:0];      

	const char *documentsDirectory = [documentsDirectoryNS cStringUsingEncoding: NSASCIIStringEncoding  ];

	NSString *iniPathNS = [documentsDirectoryNS stringByAppendingPathComponent:@"/moses.ini"]; 
	NSLog(iniPathNS);
	
	char source[1000];
	char target[1000];
	char description[1000];
	
	const char *iniPath = [iniPathNS cStringUsingEncoding: NSASCIIStringEncoding  ];
	
	int ret = LoadModel(documentsDirectory, iniPath, source, target, description);
	//NSLog(ret);
	
}



/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}

- (IBAction) doNotPressButtonWasTouched
{	
	// made red
	[doNotPressButton setTitleColor:[UIColor redColor] 
												 forState:UIControlStateNormal];
	
	NSString *sourceString = sourceText.text;
	NSLog(sourceString);
	
	const char *cstring = [sourceString UTF8String];
	char* result = (char *) malloc(sizeof(char)*1000);
	TranslateSentence(cstring,result);
	
	NSString *greeting = //[[NSString alloc] initWithFormat:@"Hello, %@!", nameString];
	[[NSString alloc] initWithUTF8String: result];
	NSLog(greeting);
	
	free(result);
	
	targetText.text = greeting;
	
	[greeting release];
	
}

@end
