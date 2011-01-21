//
//  View2.h
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface View2 : UIViewController {
	IBOutlet UITextField *txtList, *txtLoad;
	IBOutlet UIButton *btLoad;

}

@property (nonatomic, retain) IBOutlet UITextField *txtList, *txtLoad;
@property (nonatomic, retain) IBOutlet UIButton *btLoad;

- (IBAction) loadButtonWasTouched;

@end
