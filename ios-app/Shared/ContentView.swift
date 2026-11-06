//
//  ContentView.swift
//  Shared
//
//  Created by ED LEWIS on 9/15/21.
//

import SwiftUI

struct ContentView: View {
    
    @ObservedObject var bleManager = BLEManager()
    @State private var isActive = false

    var body: some View {
        NavigationView {
    
        VStack (spacing: 10) {
            NavigationLink(destination: WheelEditor(), isActive: $isActive) { }
            Text("Bluetooth Devices")
                .font(.largeTitle).frame(maxWidth: .infinity, alignment: .center)
            List(bleManager.peripherals) { peripheral in Button(action: {isActive = true; bleManager.connectToPeripheral(peripheral: peripheral.object)}){
                    HStack {
                        Text(peripheral.name)
                        Spacer()
                        Text(String(peripheral.rssi))
                    }
                }
            }.frame(height: 300)
            Spacer()
            
            Text("STATUS")
                .font(.headline)
            
            // Status goes here
            if bleManager.isSwitchedOn {
                Text("Bluetooth is switched on")
                    .foregroundColor(.green)
            }
            else {
            Text("Bluetooth is switched off")
                .foregroundColor(.red)
            }
            
            HStack {
                VStack (spacing: 10) {
                    Button(action: {
                        bleManager.startScanning()
                    }) {
                        Text("Scan")
                    }
 
                }.padding(10)
                
                Spacer()
                
                VStack (spacing: 10) {
                    Button(action: { bleManager.disconnectPeripheral()})
                    {
                        Text("Disconnect")
                    }.disabled(bleManager.isDisconnected == true)
                }.padding(10)
                
            }
            Spacer()
    
            
        }
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        Group {
            ContentView()
        }
    }
}
