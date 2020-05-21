// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify"
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

contract Fund {
    bool isOpen;
    address owner;

    constructor() public { owner = msg.sender; }

    function claim() public {
        owner = msg.sender;
    }

    function open() public {
        require(msg.sender == owner);
        isOpen = true;
    }

    function close() public {
        require(msg.sender == owner);
        isOpen = false;
    }

    function deposit() public payable { require(isOpen); }
}

contract Manager {
    Fund fund;
    bool called;
    uint prev_balance;

    constructor() public { fund = new Fund(); }

    function openFund() public {
        called = true;
        fund.open();
    }

    function snapshot() public {
        prev_balance = address(fund).balance;
    }

    function repOK() public view {
        assert(called || address(fund).balance == prev_balance);
    }
}
