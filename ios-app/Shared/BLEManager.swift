//
//  BLEManager.swift
//  ble test
//
//  Created by ED LEWIS on 9/15/21.
//

import Foundation
import CoreBluetooth

struct Peripheral: Identifiable {
    let id: Int
    let name: String
    let rssi: Int
    let object: CBPeripheral
}

class BLEManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {

    var myCentral: CBCentralManager!
    var myPeripheral: CBPeripheral!

    @Published var isSwitchedOn = false
    @Published var isDisconnected = true
    @Published var peripherals = [Peripheral]()
    
    private var txCharacteristic: CBCharacteristic!
    private var rxCharacteristic: CBCharacteristic!
    
    override init() {
        super.init()
        
        myCentral = CBCentralManager(delegate: self, queue: nil)
        myCentral.delegate = self
    }
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            isSwitchedOn = true
        }
        else {
            isSwitchedOn = false
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        var peripheralName: String!
        
        if let name = peripheral.name {
            peripheralName = name
        }
        else {
            peripheralName = "Unknown"
        }
        if (peripheralName.starts(with: "Itsy")) {

        let newPeripheral = Peripheral(id: peripherals.count, name: peripheralName, rssi: RSSI.intValue, object: peripheral)
        peripheral.delegate = self
        print(newPeripheral)
            peripherals.append(newPeripheral)
        }
    }
    
    func connectToPeripheral(peripheral: CBPeripheral) {
        self.myPeripheral = peripheral
        self.myPeripheral.delegate = self
        BlePeripheral.connectedPeripheral = peripheral
        self.myCentral.connect(peripheral, options: nil)
    }

    func disconnectPeripheral() {
        if self.myPeripheral != nil {
            self.myCentral.cancelPeripheralConnection(self.myPeripheral)
        }
    }
    
    func startScanning() {
        print("start scanning")
        self.peripherals = []
        myCentral.scanForPeripherals(withServices: nil, options: nil)
        DispatchQueue.main.asyncAfter(deadline: .now() + 5) {
            self.stopScanning()
        }
    }
    
    func stopScanning() {
        print("stopScanning")
        myCentral.stopScan()
    }
    
    func writeOutgoingValue(data: String){
        let valueString = data.data(using: .utf8)
        //change the "data" to valueString
      if let blePeripheral = BlePeripheral.connectedPeripheral {
            if let txCharacteristic = BlePeripheral.connectedTXChar {
                blePeripheral.writeValue(valueString!, for: txCharacteristic, type: CBCharacteristicWriteType.withResponse)
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        isDisconnected = false
        self.myPeripheral.discoverServices([CBUUIDs.BLEService_UUID])
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        isDisconnected = true
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        print("did discover services")
      guard let services = peripheral.services else { return }
      for service in services {
        peripheral.discoverCharacteristics(nil, for: service)
      }
      BlePeripheral.connectedService = services[0]
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {

      guard let characteristics = service.characteristics else {
          return
      }

      print("Found \(characteristics.count) characteristics.")

      for characteristic in characteristics {

        if characteristic.uuid.isEqual(CBUUIDs.BLE_Characteristic_uuid_Rx)  {

          rxCharacteristic = characteristic

          BlePeripheral.connectedRXChar = rxCharacteristic

          peripheral.setNotifyValue(true, for: rxCharacteristic!)
          peripheral.readValue(for: characteristic)

          print("RX Characteristic: \(rxCharacteristic.uuid)")
        }

        if characteristic.uuid.isEqual(CBUUIDs.BLE_Characteristic_uuid_Tx){
          txCharacteristic = characteristic
          BlePeripheral.connectedTXChar = txCharacteristic
          print("TX Characteristic: \(txCharacteristic.uuid)")
        }
      }
   }
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {

      var characteristicASCIIValue = NSString()

      guard characteristic == rxCharacteristic,

            let characteristicValue = characteristic.value,
            let ASCIIstring = NSString(data: characteristicValue, encoding: String.Encoding.utf8.rawValue) else { return }

        characteristicASCIIValue = ASCIIstring

      print("Value Received: \((characteristicASCIIValue as String))")

      NotificationCenter.default.post(name:NSNotification.Name(rawValue: "Notify"), object: "\((characteristicASCIIValue as String))")
    }
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil else {
            print("Error discovering services: error")
            return
        }
      print("Function: \(#function),Line: \(#line)")
        print("Message sent")
    }
    
}
