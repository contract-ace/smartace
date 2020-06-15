---
layout: post
title: "4. Verifying Client Properties in SmartACE"
subtitle: "An Introduction to Local Reasoning"
date: 2020-05-25 12:00:00
categories: [smartace, verification, model checking, local reasoning]
---

# 4. Verifying Client Properties in SmartACE

By Scott Wesley in collaboration Maria Christakis, Arie Gurfinkel, Xinwen Hu,
Jorge Navas, and Valentin Wüstholz.

In our [previous tutorial](3_transactions.md), we used SmartACE to identify
ownership exploits in a simple smart contract. We concluded the tutorial by
fixing the bug, and proving its absence for executions with two clients.
However, a real developer would want to verify the property holds for any
number of clients. This is known as the parameterized model checking problem,
and in SmartACE, we solve it through local reasoning [[1](#references),
[2](#references)].

Informally, the parameterized model checking problem asks whether a property
about clients is invariant for any number of clients. Local reasoning allows us
to reduce the parameterized model checking problem to a model with a fixed
number of clients. We then generalize the results to an arbitrary number of
clients. In this tutorial, we will return to the `Fund` and `Manager`
[example](3_transactions.md), and show that its proof certificate extends to any
number of clients. In the following tutorials, we will extend our technique to
contracts with mappings of client data.

**Note**: This tutorial assumes all commands are run from within the
[SmartAce container](1_installation.md). All tutorial files are available within
the container from the home directory.

## The Contract and Property

We now return to the `Manager` bundle given below. As before, we have two
smart contracts: `Fund` and `Manager`. The `Manager` constructs a new `Fund`,
for which it is initially its owner. The `Manager` exposes a single method,
`openFund()`, which allows clients to deposit Ether into `Fund`. To fix the
ownership exploit in the previous example, we have added an `ownerOnly()`
modifier to the protected interfaces of `Fund`, and have replaced `claim()` with
`releaseTo(_new)` so that `Manager` must approve ownership transfers. The full
contract is
[available here](https://github.com/ScottWe/smartace-examples/blob/master/tutorials/post-4/fund.sol).

```solidity
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

    constructor() public { fund = new Fund(); }

    function openFund() public { fund.open(); }
}
```

As before, we wish to prove the following property:

> It is always the case that if `openFund()` is not called even *once*, then the
> balance of `Manager.fund` prior to and after the last transaction remains
> unchanged.

Using the [VerX Specification Language](https://verx.ch/docs/spec.html) we
formalize the property as:

```
always(
    !(once(FUNCTION == Manager.openFund()))
    ==>
    (BALANCE(Fund) == prev(BALANCE(Fund)))
)
```

We then construct a monitor from the property. A more detailed analysis is
found in the [previous tutorial](3_transactions.md).

## Limiting Addresses in the `Manager` Bundle

In the [first tutorial](2_getting_started.md), contracts were *oblivious* to
their clients. No matter which client initiated a transaction, the outcome would
always be the same. In reality, most contracts are *client aware*. They read
from `msg.sender`, and will adjust their behaviour according. This is true of
the `Fund` contract, as it can designate a single client as its `owner`. To show
that our property holds in general, we must prove that adding an additional
client will never introduce a new interaction which violates the property.

We could tackle this problem directly by modeling every client. However, this
would not scale to bundles with even a small amount of client state. Instead, it
would be ideal to summarize the clients, and then retain only a small set of
addresses. Even in the case of `Fund`, where clients lack state, we would still
benefit from reducing the search space of interleaving client transactions.

For this reason, let's try proving the property with a fixed number of
addresses. For convenience, we will refer to the arrangement of clients and
contracts as a *network topology*, and we will refer to the subset of clients as
a *neighbourhood*. We will justify these terms in the next tutorial.

So what happens when we construct a neighbourhood? If we select too few clients,
we may lose behaviours from the original bundle (formally, this is an
*underapproximation*). For instance, if we deployed `Fund` with a single client,
the `owner` of `Fund` would never change. However, if we introduced too many
clients, this could lead to redundant traces (formally, this is *symmetry*). For
example, if we deployed `Fund` with 100 clients, there would be many identical
ways to select an initial `owner`, and then transfer to a secondary `owner`.

If we look closely at `Fund`, we will see that addresses only appear in equality
relations. An equality relation can distinguish between any two address
arguments, or between an address argument and a literal address. Note that due
to the semantics of Solidity, `address(0)` is an implicit literal of all
bundles. We are guaranteed to cover all paths of execution if we can assign a
unique value to each address in any transaction. The specific values are
unimportant, provided that we include all literals inside the neighbourhood.

In other words, we must find the maximum number of addresses used by any
transaction. We call this number the maximum *transactional address footprint*.
As the `Manager` bundle is free from address operators, we only need to count
the number of contract addresses, address variables, and address arguments in
each transaction. We find that `Fund.releaseTo(address)` has the maximum address
footprint, with two contract addresses, one address variable (`owner`) and two
address arguments (`msg.sender` and `_new`). Therefore, our neighbourhood will
have 6 addresses: `address(0)`, `address(Manager)`, `address(Manager.fund)`,
and the transactional address footprint placeholders.

### Instrumenting the Restricted Address Set

SmartACE will construct the neighbourhood automatically. We can compare
against SmartACE by runnings the following commands.

  * `solc fund.sol --bundle=Manager --c-model --output-dir=fund`
  * `cd fund ; mkdir build ; cd build`
  * `CC=clang-10 CXX=clang++-10 cmake ..`
  * `cmake --build . --target run-clang-format`

For those interested in how addresses are modelled, but do not wish to run the
tools, the full generated model is
[available here](https://github.com/ScottWe/smartace-examples/tree/master/tutorials/post-4/fund).
As before, we will open `../cmodel.c`. Let's start by inspecting the assignment
of literal addresses and contract addresses at line 121:

```cpp
global_address_literal_0 = 0;
contract_0.model_address.v = 1;
contract_1 = &contract_0.user_fund;
contract_1->model_address.v = 2;
```

This highlights several design decisions. All addresses come from a continuous
range, starting from `0`. The address at `0` is always `address(0)`. Following
this is the set of contract addresses. These are assigned statically, in the
order the contracts are constructed. The remaining addresses are reserved for
clients. To ensure the neighbourhood is always continuous, we replace address
literals with global variables. These variables are set once, as we see here
with `global_address_literal_0`. If there were any other address literals, they
would be assigned non-deterministically, to unique addresses. This captures the
case where a contract address is used as a literal. To see how the addresses are
used, we move ahead to line 135:

```cpp
sender.v = nd_range(3, 6, "sender");
```

This line highlights the benefit of our neighbourhood arrangement. By packing
together all client addresses, we can select a sender simply by selecting a
number between two bounds. For general address arguments, we can sample over the
entire neighbourhood in the same way. For example refer to line  151:

```cpp
sol_address_t arg__new = Init_sol_address_t(nd_range(0, 6, "_new"));
```

With this, SmartACE has reduced the network topology to a sufficient
neighbourhood. In the next section, we take a closer look at what assumptions
SmartACE must make to construct this neighbourhood. The reader who is only
interested in the running example can safely skip to the
[final section](#proving-the-correctness-of-fund-and-manager).

## Reasoning About Clients Through Program Syntax

So far we have seen how to construct a neighbourhood for the `Manager` bundle.
We argued that the model preserved the correctness of the bundle by appealing to
the network's topology, that is, the way in which clients are organized. We
inferred this organization by inspecting the source text. This motivates a
syntax-directed client analysis.

In particular, we can summarize our analysis of the `Manager` bundle with four
syntactic patterns. SmartACE does this analysis automatically. The patterns are
as follows:

### 1. Equality is the only address relation.

Addresses must be unordered. This ensures that given `n` addresses, we only
require `n` values to satisfy all possible comparisons. This however, comes at
the cost of many client organizations. Thankfully, Solidity was designed for
wealth transfer between arbitrary parties, so this restriction is met by major
Ethereum protocols such as the [ERC-20](https://eips.ethereum.org/EIPS/eip-20)
and the [ERC-721](https://eips.ethereum.org/EIPS/eip-721).

### 2. Address operations are never used.

Addresses must also be unstructured. This disallows many client arrangements,
such as trees. While such topologies can have uses in smart contracts, they
are
[discouraged in relevant examples](https://solidity.readthedocs.io/en/v0.5.3/solidity-by-example.html).

### 3. Address casts are never used.

This follows from the first two restrictions. Address casting is used to apply
arithmetic operators and relations to address values.

### 4. Addresses must come from inputs or named state variables.

This ensures that every transactional address footprint is bounded. Given the
first three restrictions, this ensures that the neighbourhood is bounded. In
practice, this pattern prevents iteration over addresses, such as in the
following example:

```
contract UnboundedFootprintExample {
    // ...
    function bad(address payable[] _unboundedClients) public {
        for (uint i = 0; i < _unboundedclients.length; i++) {
            applyTo(_unboundedClients[i]);
        }
    }
    // ...
}
```

Solidity
[best practices](https://solidity.readthedocs.io/en/v0.6.8/security-considerations.html#gas-limit-and-loops)
already warn against unbounded loops. Unbounded loops require unbounded gas,
which is [impossible in Ethereum](https://ethgasstation.info/blog/gas-limit/).
As for bounded loops, we can unroll them prior to analysis.

## Proving the Correctness of `Fund` and `Manager`

Now we finish with the running example. Let's start by grabbing a copy of the
[instrumented model](https://github.com/ScottWe/smartace-examples/blob/master/tutorials/post-4/instrumented/cmodel.c).
As before, we can run `cmake --build . --target verify` to obtain a proof
certificate. The [certificate](https://arieg.bitbucket.io/pdf/hcvs17.pdf) is
given in the form of an inductive program invariant. Using the invariant, we
can prove that along any program path, every assertion holds. Unfortunately, the
invariant is given with respect to LLVM-bitcode. SmartACE does not yet offer
tooling to make the certificate more readable. Interpreting the certificate
requires inspecting the LLVM-bitcode, and then mapping the registers back to
variables. You can take it on good faith that the certificate states:

  1. `Manager.fund.isOpen ==> called_openFund`
  2. `Manager.fund.owner == 1`

Lemma one is straight forward. It states that whenever the guard variable for
`Manager.fund.deposit()` is set, then `Manager.openFund()` has been called. We
know that `deposit()` is the only payable method of `Fund`, so the lemma implies
our property. However, this lemma does not ensure that other clients cannot call
`Manager.fund.open()`. It is only inductive relative to lemma two.

Lemma two is about addresses, so we must take more care in interpreting it. From
the previous section, we know that `0` maps to `address(0)`, `1` maps to
`address(Manager)`, and `2` maps to `address(Fund)`. Then we can read lemma two
as `Manager.fund.owner == address(Manager)`. This means that the ownership of
`Manager.fund` is invariant to any sequence of transactions from any sequence of
arbitrary clients (i.e., addresses `3` and `4`). We know that `Manager` cannot
start a transaction, and that only `Manager.fund.owner` can call
`Manager.fund.open()`, so this lemma blocks all counterexamples to lemma one.

## Conclusion

In this tutorial, we learned how SmartACE generalizes bounded proofs to
unbounded numbers of clients. We outlined the requirements a smart contract must
satisfy for this technique to apply. We also learned how the way in which
SmartACE encodes the addresses can be used to simplify the model. In the next
tutorial, we will extend these techniques to smart contracts with mapping
variables.

## References

  1. Gurfinkel, A., Meshman, Y., Shoham, S.: SMT-based verification of
     parameterized systems. FSE. **24**, 338-348 (2016). DOI:
     https://doi.org/10.1145/2950290.2950330

  2. Namjoshi, K.S., and Trefler, R.J.: Parameterized compositional model
     checking. TACAS. **22**, 589-606 (2016). DOI:
     https://doi.org/10.1007/978-3-662-49674-9_39
