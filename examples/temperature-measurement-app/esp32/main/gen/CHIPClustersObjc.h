/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// THIS FILE IS GENERATED BY ZAP

#ifndef CHIP_CLUSTERS_H
#define CHIP_CLUSTERS_H

#import <Foundation/Foundation.h>

typedef void (^ResponseHandler)(NSError * _Nullable error, NSDictionary * _Nullable values);

@class CHIPDevice;

NS_ASSUME_NONNULL_BEGIN

@interface CHIPBasic : NSObject

- (nullable instancetype)initWithDevice:(CHIPDevice *)device endpoint:(uint8_t)endpoint queue:(dispatch_queue_t)queue;
- (BOOL)resetToFactoryDefaults:(ResponseHandler)completionHandler;

- (BOOL)readAttributeZclVersion:(ResponseHandler)completionHandler;
- (BOOL)readAttributePowerSource:(ResponseHandler)completionHandler;
- (BOOL)readAttributeClusterRevision:(ResponseHandler)completionHandler;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

NS_ASSUME_NONNULL_BEGIN

@interface CHIPTemperatureMeasurement : NSObject

- (nullable instancetype)initWithDevice:(CHIPDevice *)device endpoint:(uint8_t)endpoint queue:(dispatch_queue_t)queue;

- (BOOL)readAttributeMeasuredValue:(ResponseHandler)completionHandler;
- (BOOL)configureAttributeMeasuredValue:(uint16_t)minInterval
                            maxInterval:(uint16_t)maxInterval
                                 change:(int16_t)change
                      completionHandler:(ResponseHandler)completionHandler;
- (BOOL)reportAttributeMeasuredValue:(ResponseHandler)reportHandler;
- (BOOL)readAttributeMinMeasuredValue:(ResponseHandler)completionHandler;
- (BOOL)readAttributeMaxMeasuredValue:(ResponseHandler)completionHandler;
- (BOOL)readAttributeClusterRevision:(ResponseHandler)completionHandler;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif /* CHIP_CLUSTERS_H */