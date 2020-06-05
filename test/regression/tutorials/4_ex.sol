// RUN: %solc %s --c-model --output-dir=%t
// RUN: cd %t
// RUN: cmake -DSEA_PATH=%seapath
// RUN: make verify | OutputCheck %s --comment=//
// RUN: make cex
// [ ! -f cex.ll ]
// CHECK: ^unsat$

contract Fund {
    bool isOpen;
    address owner;

    constructor() public { owner = msg.sender; }

    modifier ownerOnly() { require(owner == msg.sender); _; }
    function releaseTo(address _new) public ownerOnly { owner = _new; }

    function open() public ownerOnly { isOpen = true; }
    function close() public ownerOnly { isOpen = false; }
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
