// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make fuzz

contract Crowdsale {
    uint raised;
    uint goal;
    uint deadline;

    bool finished;
    bool canceled;

    constructor(uint _goal) public {
        goal = _goal;
        deadline = now + 365 days;
        require(now < deadline);
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
