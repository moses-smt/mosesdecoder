//
//  FirstViewController.h
//  moses
//
//  Created by Hieu Hoang on 19/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface FirstViewController : UIViewController {
	UIButton *doNotPressButton;
	UIAlertView *alertView;
	UITextField *sourceText, *targetText;
	
}

@property (nonatomic, retain) IBOutlet UIButton *doNotPressButton;
@property (nonatomic, retain) IBOutlet UITextField *sourceText, *targetText;

- (IBAction) translateButtonWasTouched;
- (IBAction) infoButtonWasTouched;

@end

