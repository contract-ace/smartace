// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify | OutputCheck %s --comment=//
// RUN: make cex
// [ ! -f cex.ll ]
// CHECK: ^unsat$

contract Crowdsale {
    uint raised;
    uint goal;
    uint deadline;

    bool finished;
    bool canceled;

    constructor(uint _goal) public {
        goal = _goal;
        deadline = now + 365 days;
    }

    function invest() public payable {
        require(now <= deadline);
        raised += msg.value;
    }

    function finish() public {
        require(address(this).balance >= goal);
        finished = true;
    }

    function cancel() public {
        require(address(this).balance < goal && now > deadline);
        canceled = true;
    }

    function repOK() public view {
        assert(!(finished && canceled));
    }
}
