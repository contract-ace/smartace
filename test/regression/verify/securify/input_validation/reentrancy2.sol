// RUN: %solc %s --c-model --output-dir=%t --bundle A
// RUN: cd %t
// RUN: %cmake -DSEA_PATH=%seapath -DSEA_ARGS="--verify" %buildargs
// RUN: make verify 2>&1 | OutputCheck %s --comment=//
// CHECK: unsat

/**
 * Note: we do not yet support unbounded arrays. The code's use of arrays was
 * not crucial to the correctness of input validation. To allow for this test to
 * work the array was omitted.
 *
 * Inheritance was also inlined.
 *
 * TODO: replace array
 * TODO: replace inheritance and safe math
 */

contract A {
  bool _checked_add_a;
  bool _checked_add_b;
  bool _checked_sub_a;
  bool _checked_sub_b;

  mapping (address => uint256) public deposited;
  address public owner;
  address payable public wallet;

  constructor(address payable _wallet) public {
    require(_wallet != address(0));
    wallet = _wallet;
    owner = msg.sender;
  }

  modifier onlyOwner() {
    require(msg.sender == owner);
    _;
  }

  function transferOwnership(address newOwner) public onlyOwner {
    // INSTRUMENTATION (INITIALIZE FLAGS)
    bool _checked_newOwner = false;
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (CONTROL FLOW : require)
    _checked_newOwner = true;
    require(newOwner != address(0));
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (CHECK BEFORE RETURN)
    assert(_checked_newOwner);
    owner = newOwner;
    // INSTRUMENTATION (END)
  }

  function deposit(address payable beneficiary) public payable onlyOwner {
    // INSTRUMENTATION (INITIALIZE FLAGS)
    bool _tmpchecked_add_a = _checked_add_a;
    bool _tmpchecked_add_b = _checked_add_b;
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (CALL)
    _checked_add_a = false;
    _checked_add_b = false;
    deposited[beneficiary] = add(deposited[beneficiary], msg.value);
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (RESET FLAGS)
    _checked_add_a = _tmpchecked_add_a;
    _checked_add_b = _tmpchecked_add_b;
    // INSTRUMENTATION (END)
  }

  function release(address payable beneficiary, uint256 overflow) public onlyOwner {
    // INSTRUMENTATION (INITIALIZE FLAGS)
    bool _tmpchecked_sub_a = _checked_sub_a;
    bool _tmpchecked_sub_b = _checked_sub_b;
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (CALL)
    _checked_sub_a = false;
    _checked_sub_b = false;
    uint256 amount = sub(deposited[beneficiary], overflow);
    // INSTRUMENTATION (END)
    deposited[beneficiary] = 0;

    wallet.transfer(amount);
    if (overflow > 0) {
      beneficiary.transfer(overflow);
    }
    // INSTRUMENTATION (RESET FLAGS)
    _checked_sub_a = _tmpchecked_sub_a;
    _checked_sub_b = _tmpchecked_sub_b;
    // INSTRUMENTATION (END)
  }

  function refund(address payable beneficiary) public onlyOwner {
    uint256 depositedValue = deposited[beneficiary];
    deposited[beneficiary] = 0;

    beneficiary.transfer(depositedValue);
  }

  function sub(uint256 a, uint256 b) internal returns (uint256) {
    // INSTRUMENTATION (CONTROL FLOW : REQUIRE)
    _checked_sub_a = true;
    _checked_sub_b = true;
    require(b <= a);
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (CHECK BEFORE RETURN)
    assert(_checked_sub_a);
    assert(_checked_sub_b);
    return a - b;
    // INSTRUMENTATION (END)
  }

  function add(uint256 a, uint256 b) internal returns (uint256 c) {
    c = a + b;
    // INSTRUMENTATION (CONTROL FLOW : REQUIRE)
    // Note: this requires taint analysis...
    _checked_add_a = true;
    _checked_add_b = true;
    require(c >= a);
    // INSTRUMENTATION (END)
    // INSTRUMENTATION (CHECK BEFORE RETURN)
    assert(_checked_add_a);
    assert(_checked_add_b);
    return c;
    // INSTRUMENTATION (END)
  }
}

