import Foundation
import Combine

class WheelData : ObservableObject {

    @Published var dualMode = false
    @Published var dummy = false

    class Wheel: ObservableObject, Identifiable {
        var id: Int
        var mode: Int
        var subMode: Array<Int>
        var reversed: Bool
        
        init(id: Int) {
            self.id = id
            self.mode = 0
            self.subMode = Array(repeating: 0, count: ModeConstants.modes.count)
            self.reversed = false
        }
    }
    
    @Published var wheelOne = Wheel( id: 0 )
    @Published var wheelTwo = Wheel( id: 1 )
    
    init() {
    }

    func switchDualMode() {
        dualMode = !dualMode
    }
    
    func hitDummy() {
        dummy = !dummy
    }
    
    func addZero(_ mode: Int) -> String {
        mode < 10 ? "0\(mode)" : "\(mode)"
    }
    
    func copy() {
        wheelTwo.mode = wheelOne.mode
        wheelTwo.subMode = wheelOne.subMode
        hitDummy()
    }
    
//    func prepWheelForSend(_ wheel: Wheel) -> String {
//        "M\(wheel.mode)\(wheel.subMode[wheel.mode])"
//    }
    
    func secondChar(_ wheel: Wheel) -> Character {
        let value = wheel.subMode[wheel.mode] + (wheel.reversed ? 30 : 0 ) + 48
        let u = UnicodeScalar(value)
        let char = Character(u!)
        return char
    }
    
    func prepWheelForSend(_ wheel: Wheel) -> String {
        "M\(wheel.mode)\(secondChar(wheel))"
    }
    
    func makeSendString() -> String {
        let result = "\(prepWheelForSend(wheelOne))X"
//        let result = "\(prepWheelForSend(wheelOne))\(prepWheelForSend(wheelTwo))"
        print(result)
        return result
    }

}
