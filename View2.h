//
//  View2.h
//  moses
//
//  Created by Hieu Hoang on 21/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface View2 : UIViewController 
										<UITableViewDelegate, UITableViewDataSource>
{
	IBOutlet UITextField *txtURL;
	IBOutlet UIButton *btLoad, *btDownload;
	IBOutlet UIActivityIndicatorView *busyIndicator;
	IBOutlet UITableView *tableView;
	UIAlertView *alertView;
	NSMutableArray *folderNames;
	
	NSTimer *timer;

}

@property (nonatomic, retain) IBOutlet UITextField *txtURL;
@property (nonatomic, retain) IBOutlet UIButton *btLoad, *btDownload;
@property (nonatomic, retain) IBOutlet UIActivityIndicatorView *busyIndicator;

@property (nonatomic, retain) NSMutableArray *folderNames;

- (IBAction) downloadButtonWasTouched;

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section;
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath;

@end
