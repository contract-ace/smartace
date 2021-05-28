// RUN: %solc %s --c-model --bundle A --output-dir=%t
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make cex
// RUN: [ -f cex.ll ]
// RUN: make witness | OutputCheck %s --comment=//
// CHECK: __VERIFIER_error

library Auction {
    struct Record {
        uint256 max;
        mapping (address => uint256) bids;
    }

    function bid(Record storage record, address account, uint256 amt) internal {
        require(account != address(0));
        require(record.max == 0 || record.bids[account] < record.max);
        require(amt > record.max);

        record.max = amt;
        record.bids[account] = amt;
    }

    function get(Record storage record, address account) internal view returns (uint256) {
        require(account != address(0));
        return record.bids[account];
    }
}

contract A {
    using Auction for Auction.Record;

    Auction.Record private _records;

    function bid(uint256 amt) public {
        uint pre = _records.get(msg.sender);
        _records.bid(msg.sender, amt);
        uint post = _records.get(msg.sender);
        assert(pre == 0 || pre == post);
    }
}
