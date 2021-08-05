// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: cmake . -DCMAKE_CXX_COMPILER=%clangpp -DCMAKE_C_COMPILER=%clangc -DKLEE_PATH=%kleepath -DKLEE_LIB=%kleelib -DKLEE_MAX_TIME=5min %buildargs
// RUN: make symbex 2>&1 | OutputCheck %s --comment=//
// CHECK-NOT: ERROR

contract A {
    uint initAt;

    constructor() public { initAt = block.number; }

    function check() public view {
        assert(address(this).balance == 0 && initAt <= block.number);
    }
}
