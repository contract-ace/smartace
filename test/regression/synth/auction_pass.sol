// RUN: %solc %s --c-model --output-dir=%t --bundle Manager --invar-type singleton --invar-rule checked --invar-infer=on --invar-stateful=on
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/*
 * Example from https://github.com/contract-ace/smartace-2020-experiments.
 */

contract Auction {
    bool isOpen;
    address owner;

    uint256 maxBid;

    mapping(address => uint) bids;

    constructor() public { owner = msg.sender; }

    // Access controls.
    modifier ownerOnly() { require(owner == msg.sender); _; }
    function releaseTo(address _new) public ownerOnly { owner = _new; }
    function open() public ownerOnly { isOpen = true; }
    function close() public ownerOnly { isOpen = false; }

    // Place a bid.
    function deposit() public payable {
        uint256 bid = bids[msg.sender] + msg.value;
        require(isOpen);
        require(bid > maxBid);
        bids[msg.sender] = bid;
        maxBid = bid;
    }

    // Withdraw a losing bid.
    function withdraw() public payable {
        uint256 bid = bids[msg.sender];
        require(isOpen);
        require(bid < maxBid);
        bids[msg.sender] = 0;
        msg.sender.transfer(bid);
    }
     
    // Property.
    function check() public view {
        assert(bids[msg.sender] <= maxBid);
    }
}

contract Manager {
    Auction auction;

    constructor() public { auction = new Auction(); }

    function openAuction() public { auction.open(); }
}
