/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2007-2013 Andrey Prygunkov <hugbug@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Revision: 807 $
 * $Date: 2013-08-31 23:14:39 +0200 (Sat, 31 Aug 2013) $
 *
 */

#import <Cocoa/Cocoa.h>
#import "RPC.h"

@implementation RPC

NSString* rpcUrl;

- (id)initWithMethod:(NSString*)method
			receiver:(id)receiver
			 success:(SEL)successCallback
			 failure:(SEL)failureCallback {
	NSString* urlStr = [rpcUrl stringByAppendingString:method];
	self = [super initWithURLString:urlStr receiver:receiver success:successCallback failure:failureCallback];
	return self;
}

+ (void)setRpcUrl:(NSString*)url {
	rpcUrl = url;
}

- (void)success {
    NSError *error = nil;
    id dataObj = [NSJSONSerialization
				  JSONObjectWithData:data
				  options:0
				  error:&error];
	
    if (error || ![dataObj isKindOfClass:[NSDictionary class]]) {
		/* JSON was malformed, act appropriately here */
		failureCode = 999;
		[self failure];
	}

	id result = [dataObj valueForKey:@"result"];
	SuppressPerformSelectorLeakWarning([_receiver performSelector:_successCallback withObject:result];);
}

@end
