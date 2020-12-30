// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake . -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DSEA_PATH=%seapath -DFUZZ_RUNS=5000
// RUN: cmake --build . --target fuzz

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
        uint old_raised = raised;
        require(now <= deadline);
        raised += msg.value;
        require(raised > old_raised);
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
