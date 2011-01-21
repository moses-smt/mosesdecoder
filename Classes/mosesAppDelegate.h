//
//  mosesAppDelegate.h
//  moses
//
//  Created by Hieu Hoang on 19/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class mosesViewController;

@interface mosesAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
	UITabBarController *tabBarController;

    mosesViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet mosesViewController *viewController;
@property (nonatomic, retain) IBOutlet UITabBarController *tabBarController;

@end

