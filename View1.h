//
//  View1.h
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface View1 : UIViewController {
	IBOutlet UITextField *txtSource, *txTarget;
	IBOutlet UIButton *btTranslate;
	UIAlertView *alertView;
	
}

@property (nonatomic, retain) IBOutlet UITextField *txtSource, *txTarget;
@property (nonatomic, retain) IBOutlet UIButton *btTranslate;

@property (nonatomic, retain) IBOutlet UITabBarController *tab;

- (IBAction) translateButtonWasTouched;
- (IBAction) translateNow;

@end
