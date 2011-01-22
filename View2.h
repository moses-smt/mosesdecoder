//
//  View2.h
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface View2 : UIViewController {
	IBOutlet UITextField *txtList, *txtLoad, *txtURL;
	IBOutlet UIButton *btLoad, *btDownload;
	UIAlertView *alertView;

}

@property (nonatomic, retain) IBOutlet UITextField *txtList, *txtLoad, *txtURL;
@property (nonatomic, retain) IBOutlet UIButton *btLoad, *btDownload;

- (IBAction) loadButtonWasTouched;
- (IBAction) downloadButtonWasTouched;

@end
