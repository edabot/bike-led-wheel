//
//  WheelEditor.swift
//  ble test (iOS)
//
//  Created by ED LEWIS on 9/16/21.
//

import SwiftUI

struct WheelEditor: View {
    @ObservedObject var bleManager = BLEManager()    
    @StateObject var wheelData: WheelData = WheelData()

    var body: some View {
        GeometryReader{ geometry in
          VStack {
            HStack {
                Button(action: {wheelData.switchDualMode()}) {
                    Text("Dual mode")
                }
                
            }
            Spacer()
            HStack(alignment: .top) {
                Spacer()
                WheelControlOne()
                    .frame(width: geometry.size.width/2.5)
                        .clipped()
                Spacer()
                WheelControlTwo()
                    .frame(width: geometry.size.width/2.5)
                        .clipped()
                Spacer()
            }
            Spacer()
            Button(action: { bleManager.writeOutgoingValue(data:  wheelData.makeSendString()) }) {
                Text("Send It")
            }
       
            Spacer()

          }
          .environmentObject(wheelData)
        }
      }
}

struct WheelController_Previews: PreviewProvider {
    static var previews: some View {
        WheelEditor()
    }
}

struct WheelControlOne: View {

    @EnvironmentObject var wheelData: WheelData

    var body: some View {
        VStack{
            Text("Wheel One")

            Picker(selection: $wheelData.wheelOne.mode, label: /*@START_MENU_TOKEN@*/Text("Picker")/*@END_MENU_TOKEN@*/) {
            ForEach (0 ..< ModeConstants.modes.count) { mode in
                Text(ModeConstants.modes[mode].name)
               }
            }
            .frame(height: 100)
            .clipped()
        
        Picker(selection: $wheelData.wheelOne.subMode[wheelData.wheelOne.mode], label: /*@START_MENU_TOKEN@*/Text("Picker")/*@END_MENU_TOKEN@*/) {
            ForEach (ModeConstants.subModes[wheelData.wheelOne.mode], id: \.id) { item in
                Text(item.name)
               }
        }
        .frame(height: 150)
        .clipped()
            Toggle(isOn: $wheelData.wheelOne.reversed) {
                Text("Reverse")
            }
            .padding()
        }
    }
}

struct WheelControlTwo: View {

    @EnvironmentObject var wheelData: WheelData

    var body: some View {
        VStack{
            Text("Wheel Two")
        Picker(selection: $wheelData.wheelTwo.mode, label: /*@START_MENU_TOKEN@*/Text("Picker")/*@END_MENU_TOKEN@*/) {
            ForEach (0 ..< ModeConstants.modes.count) { mode in
                Text(ModeConstants.modes[mode].name)
               }
        }
        .frame(height: 100)
        .clipped()
            Picker(selection: $wheelData.wheelTwo.subMode[wheelData.wheelTwo.mode], label: /*@START_MENU_TOKEN@*/Text("Picker")/*@END_MENU_TOKEN@*/) {
                ForEach (ModeConstants.subModes[wheelData.wheelTwo.mode], id: \.id) { item in
                Text(item.name)
                }
        }
            .frame(height: 150)
            .clipped()
            Toggle(isOn: $wheelData.wheelTwo.reversed) {
                Text("Reverse")
            }
            .padding()
            Button(action: { wheelData.copy()} ) {
                Text("Copy")
            }
        }
    }
}
