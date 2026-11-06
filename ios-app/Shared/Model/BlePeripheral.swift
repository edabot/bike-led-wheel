//
//  BlePeripheral.swift
//  ble test (iOS)
//
//  Created by ED LEWIS on 9/16/21.
//

import Foundation
import CoreBluetooth

class BlePeripheral {
 static var connectedPeripheral: CBPeripheral?
 static var connectedService: CBService?
 static var connectedTXChar: CBCharacteristic?
 static var connectedRXChar: CBCharacteristic?
}
