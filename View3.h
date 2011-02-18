//
//  View3.h
//  moses
//
//  Created by Hieu Hoang on 22/01/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface View3 : UIViewController {
	IBOutlet UIButton *btDownload, *btRefresh;
	IBOutlet UITextField *txtURL;
	IBOutlet UIActivityIndicatorView *busyIndicator;
	IBOutlet UITableView *tableView;

	UIAlertView *alertView;
	NSTimer *timer;

	NSMutableArray *folderNames;

	NSIndexPath *selectedIndexPath;
}

@property (nonatomic, retain) IBOutlet UIButton *btDownload, *btRefresh;
@property (nonatomic, retain) IBOutlet UITextField *txtURL;
@property (nonatomic, retain) IBOutlet UIActivityIndicatorView *busyIndicator;
@property (nonatomic, retain) IBOutlet UITableView *tableView;

@property (nonatomic, retain) NSMutableArray *folderNames;

- (IBAction) downloadButtonWasTouched:(id)sender;
- (IBAction) refreshButtonWasTouched:(id)sender;

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section;
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath;

@end
