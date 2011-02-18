//
//  mosesAppDelegate.m
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "mosesAppDelegate.h"
#import "View1.h"
#import "View2.h"
#import "CFunctions.h"
extern "C" {
#import "minzipwrapper.h"
}

@implementation mosesAppDelegate

@synthesize window;
@synthesize tabBarController;


#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    
	// Override point for customization after application launch.

	// Add the tab bar controller's view to the window and display.
	[self.window addSubview:tabBarController.view];
	[self.window makeKeyAndVisible];

	self.tabBarController.selectedIndex = 1;

	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	
	NSString *isInit = [prefs stringForKey:@"init"];
	
	if (isInit == nil)
	{
		NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
		bundlePath = [bundlePath stringByAppendingPathComponent:@"/en-ht.zip"];
		NSLog(bundlePath);
		
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *documentsDirectoryNS = [paths objectAtIndex:0];      
		NSString *fullPathNS = [documentsDirectoryNS stringByAppendingPathComponent:@"/en-ht.zip"];
		NSLog(fullPathNS);

		[[NSFileManager defaultManager] copyItemAtPath:bundlePath toPath:fullPathNS error:nil];
		
		const char *fullPath = [fullPathNS cStringUsingEncoding: NSASCIIStringEncoding  ];
		const char *docPathCtr = [documentsDirectoryNS cStringUsingEncoding: NSASCIIStringEncoding  ];
		
		Unzip(fullPath, docPathCtr);
		remove(fullPath);			
		
		[prefs setValue:@"0" forKey: @"init"];
		[prefs synchronize];
	}
		
	
	return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
     */
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    /*
     Called as part of  transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
     */
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
}


- (void)applicationWillTerminate:(UIApplication *)application {
    /*
     Called when the application is about to terminate.
     See also applicationDidEnterBackground:.
     */
}


#pragma mark -
#pragma mark UITabBarControllerDelegate methods
/*
// Optional UITabBarControllerDelegate method.
- (void)tabBarController:(UITabBarController *)tabBarController didSelectViewController:(UIViewController *)viewController {
}


// Optional UITabBarControllerDelegate method.
- (void)tabBarController:(UITabBarController *)tabBarController didEndCustomizingViewControllers:(NSArray *)viewControllers changed:(BOOL)changed {
}
*/


#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    /*
     Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
     */
}


- (void)dealloc {
    [tabBarController release];
    [window release];
    [super dealloc];
}

@end

