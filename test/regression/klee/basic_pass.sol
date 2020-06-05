// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DKLEE_PATH=%kleepath -DKLEE_LIB=%kleelib
// RUN: make symbex 2>&1 | OutputCheck %s --comment=//
// CHECK-NOT: ERROR

contract A {
    uint initAt;

    constructor() public { initAt = block.number; }

    function check() public view {
        assert(address(this).balance == 0 && initAt <= block.number);
    }
}
