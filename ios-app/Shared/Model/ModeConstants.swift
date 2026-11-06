//
//  ModeConstants.swift
//  wheel2
//
//  Created by ED LEWIS on 8/13/21.
//

struct Mode: Identifiable {
    var id: Int
    var name: String
}

struct ModeConstants {
    static let modes = [
        Mode(id: 0, name: "pattern"),
        Mode(id: 1, name: "fireball"),
        Mode(id: 2, name: "sparks"),
        Mode(id: 3, name: "double fireball")]
    static let subModes = [
        [Mode(id: 0, name: "green waves"),
         Mode(id: 1, name: "rgb waves"),
         Mode(id: 2, name: "cmy chase"),
         Mode(id: 3, name: "smiley face"),
         Mode(id: 4, name: "disney"),
         Mode(id: 5, name: "GM fireball"),
         Mode(id: 6, name: "rainbow flag"),
         Mode(id: 7, name: "trans flag"),
         Mode(id: 8, name: "flag italy"),
         Mode(id: 9, name: "flag ireland"),
         Mode(id: 10, name: "flag france"),
         Mode(id: 11, name: "OB fireball"),
         Mode(id: 12, name: "rainbow"),
         Mode(id: 13, name: "BY fireball"),
         Mode(id: 14, name: "candy cane")],
        // fireball
        [Mode(id: 0, name: "yellow/red"),
         Mode(id: 1, name: "green/blue"),
         Mode(id: 2, name: "purple")],
        //sparks
        [Mode(id: 0, name: "yellow/red"),
         Mode(id: 1, name: "cyan/green"),
         Mode(id: 2, name: "purple"),
         Mode(id: 3, name: "green/blue")],
        //double fireball
        [Mode(id: 0, name: "yellow/red"),
         Mode(id: 1, name: "green/blue"),
         Mode(id: 2, name: "purple")]
    ]

}
