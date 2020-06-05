// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DKLEE_PATH=%kleepath -DKLEE_LIB=%kleelib
// RUN: make symbex 2>&1 | OutputCheck %s --comment=//
// CHECK: ERROR

contract A {
    uint initAt;

    constructor() public { initAt = block.number; }

    function check() public payable {
        assert(msg.value == 0 || initAt == block.number);
    }
}
