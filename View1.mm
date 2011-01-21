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
	NSLog(@"translate");
	// made red
	[btTranslate  setTitleColor:[UIColor redColor] 
												 forState:UIControlStateNormal];
	
	NSString *sourceString = txtSource.text;
	NSLog(sourceString);
	
	const char *cstring = [sourceString UTF8String];
	char* result = (char *) malloc(sizeof(char)*1000);
	TranslateSentence(cstring,result);
	
	NSString *greeting = //[[NSString alloc] initWithFormat:@"Hello, %@!", nameString];
	[[NSString alloc] initWithUTF8String: result];
	NSLog(greeting);
	
	free(result);
	
	txTarget.text = greeting;
	
	[greeting release];
}

- (void) alertView:(UIAlertView *)alertView
didDismissWithButtonIndex:(NSInteger) buttonIndex
{
	NSLog(@"button=%i", buttonIndex);
	NSLog(@"alertView=%i", alertView);
	
	//exit(1);	
}

@end
