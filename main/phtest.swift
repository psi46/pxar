#!/usr/bin/env xcrun swift

// ----------------------------------------------------------------------
// ../main/phtest.swift phmin=2..10 phmax=247..255 > phtest
// ----------------------------------------------------------------------

import Foundation

// ----------------------------------------------------------------------
func dumpTestPoint(_ minv: Int, _ maxv: Int, _ idx: Int) -> String {
    var tpString = ""
    tpString +=
      """
      #Index: \(idx)
      setdac phoffset 185
      setdac phscale  65
      Ph:optimize phmin=\(minv):phmax=\(maxv)
      GainPedestal dumphists=1
      """
    return tpString
}


// ----------------------------------------------------------------------
func range(_ toparse: String) -> [Int] {
    var result = [Int]()
    // -- find first number (delimited by '=+1' and '.-1'
    var idx = toparse.firstIndex(of: "=") ?? toparse.endIndex
    let idx1 = toparse.index(idx, offsetBy: 1)
    idx =  toparse.firstIndex(of: ".") ?? toparse.endIndex
    let n1 = toparse[idx1..<idx]
    // -- find second number
    let idx2 = toparse.index(idx, offsetBy:2)
    let n2 = toparse[idx2..<toparse.endIndex]
    //    print("parsing: n1->\(n1)<- n2->\(n2)<-")
    result.insert(Int(n1) ?? 0, at:0)
    result.insert(Int(n2) ?? 0, at:1)
    return result

}


// ----------------------------------------------------------------------
func main() {

    var vPhmin = [Int]()
    var vPhmax = [Int]()
    for (index, value) in CommandLine.arguments.enumerated() {
        if index == 0 {continue}
        if let _ = value.range(of: "phmin", options: .regularExpression) {
            //            print("calling range(\(value))")
            vPhmin = range(value)
        }
        if let _ = value.range(of: "phmax", options: .regularExpression) {
            //            print("calling range(\(value))")
            vPhmax = range(value)
        }
    }

    guard vPhmax.count > 0
    else {
        print("no phmax range given")
        return
    }

    guard vPhmin.count > 0
    else {
        print("no phmin range given")
        return
    }

    var card = [String]()
    var idx = 1
    for phmin in vPhmin[0]...vPhmin[1] {
        for phmax in vPhmax[0]...vPhmax[1] {
            //            print("phmin = \(phmin) phmax = \(phmax)")
            card.append(dumpTestPoint(phmin, phmax, idx))
            idx += 1
        }
    }
    card.append("q")
    for line in card {
        print("\(line)")
    }
}

// ----------------------------------------------------------------------
main()
exit(EXIT_SUCCESS)
// ----------------------------------------------------------------------
