
Algorithm for Address Search
============================

.. default-role:: math

.. math::

  \usepackage{amssymb,amsmath}
  \usepackage{amssymb,amsmath}

The prpoperty map is a single table (a 1 dimensional array) in which 
each each entry `E` defines a region within the space by a low address 
and a high address `E_{lo}` and `E_{hi}` (inclusive values). The array 
entries are non-overlapping and sorted in address order. Thus a 
straightforward binary search identifies whether an arbitrary 
address falls within one of these entries. If no entry is found then 
the address is not within the device.

Each single property in the device has a corresponding entry in this 
table for which both the low address and high address are the same.

An array property likewise may form a single entry in which the low 
and high addresses are the extents of the array. However, array 
properties may be sparse: if the increment of the array `I` - or the 
smallest increment for multidimensional arrays `I_min` - is greater than one 
the array contains holes. Furthermore, these holes may legitimately 
be occupied by other properties. For example, it is common for 
arrays to be interleaved and this interleaving can extend to 
multiple multidimensioned arrays.

Thus, having found that an address falls within the extents of an 
array, it may need further testing to find whether it is actually a 
member, or whether it falls within a hole, and in the case of 
interleaved arrays it may need testing for membership of more than 
one array.

To handle these cases, each entry in the address table contains a 
test counter ntests. When an address falls within the range defined 
by the entry, ntests indicates how many array properties overlap in 
that region and need to be tested for membership.

If `ntests = 0` then it means that the address region has no holes 
and all its address(s) unambiguously belong to a single property so 
no further tests are needed and we have found the propery. This is 
always the case for singular properties where `E_{lo}` and `E_{hi}` are the 
same. It is also the case for array properties which occupy every 
address within their range (For example a single dimensional array with 
an increment of 1).

If `ntests = 1` then there is only one array property occupying this 
region but its increments are such that it does not occupy every 
address so a further test is required to establish membership.

If `ntests > 1` then we have interleaving and must test for membership 
of multiple property ranges.

If an entry is found then it leads to one or more property addresses 
which need to be further tested. Testing whether the address matches 
a property proceeds as follows:

If `E_{lo} = E_{hi}` then we have a single property which must match.

Otherwise we have a range which includes one or more array 
properties and the Entry identifies a list of the properties to be 
tested. In many cases there will be just one array property in the 
list, but where overlapping arrays occur, it may be necessary to 
test multiple properties. Furthermore, with multi-dimensional arrays 
the test is not trivial. More on testing below.

In generating the table, there is scope for different compromises 
over the granularity with which the table is generated. Consider two 
overlapping arrays `P` and `Q`. Using the notation given in the DMP spec 
to represent an address {base, inc, count}:
``P = {100, 10, 5}   Q = {125, 10, 5}`` giving in order::

  P[0] = 100
  P[1] = 110
  P[2] = 120
  Q[0] = 125
  P[3] = 130
  Q[1] = 135
  P[4] = 140
  Q[2] = 145
  Q[3] = 155
  Q[4] = 165

We have the choice here of making a single entry::

  1. from 100 to 165 - test for P and Q

or we can create three entries::

  1. from 100 to 120 - test for P
  2. from 125 to 140 - test for P and Q
  3. from 145 to 165 test for Q

The second strategy is usually preferred as the binary search 
through the whole space can be optimised as a fast procedure whilst 
iterating through tests for array membership is onerous. However, if 
space is at a premium, or in certain worst case conditions, the 
former strategy may be preferable. However, this choice of strategy 
is made in building the search table. The process of testing a 
property for membership of the device is the same in both cases: the 
address leads to a list of zero or more properties to be tested for 
a match and those properties must be tested in turn.

Tests for property match.
-------------------------

Terms.
~~~~~~

The address we are testing for match with the property is `A`

A property has an arbitrary number `N` of dimensions `d_i, 0 ≤ i ≤ N` we
call these `d_0, d_1 ... d_i ... d_N` each of 
which is specified by an increment `I_0 ... I_i ... I_N` and a count. For the 
algorithms here we will use the range limit `r` instead of the count. `r= 
count - 1` thus the maximum inclusive value of property address `P` due to 
dimension `i` is `Pmax_i = Pbase + r_i I_i`

A generic member of the array *P* is `P_x` where:

  1. `P_x = Pbase + x_0 I_0 + x_1 I_1 + ... + x_N I_N`

For `N = 0` we have a single point property and the test is trivial: ``A - Pbase == 0``

For `N = 1` we have a linear array, but if increment `I_0 > 1` then there 
are holes so we need to know whether `A - Pbase` is an integer 
multiple of `I_0`. The simplest test is to use the modulus: ``(A - Pbase) % i[0] == 0``

For higher dimensions testing becomes more complex. The range of `P` 
is given by:

  2. `Pbase ≤ P ≤ Pbase + I_0 r_0 + I_1 r_1 + ... + i_N r_N`

Now recalling (1) we want to test ``A == Px`` but we do not know the 
values for `x_0, x_1` etc. Iterating through all `x0, x_1, x_2 ... x_N` is not
practicable for large arrays. In most cases increments and counts 
are chosen such that there is no overlap between dimensions (in a 
two dimensional case this is the same as saying that one row does 
not overlap with the next or previous). However there is no 
guarantee that this isn't the case. What we do know though provides 
limits on `x_0 ... x_N`.

First by subtracting Pbase from all calculations we can simplify so:

  3.  `Ao = A - Pbase`

Assume `A` is in `P` then for some valid set of `x_0 ... x_N`:

  4.  `Ao = x_0 I_0 + x_1 I_1 + ... + x_N I_N`

Solving for an individual `x_i`

  5.  `x_i I_i = Ao - (x_0 I_0 + x1 I_1 + ..` [miss out `x_i I_i`] `.. + x_N I_N)`

The largest value possible of `x_i` for a given `Ao` is when all other `x` 
are zero and the bracketed term in 5 becomes zero:

  6.  `max( x_i ) = Ao/I_i`

but we know that `x_i ≤ r_i` and depending on `Ao` one or other of these 
may apply so for a given `Ao` and for each dimension the top limit for
`x_i` we will call `t_i` so:

  7.  `t_i = min( r_i, Ao/I_i )`

We can now calculate `t_i` for each dimension. Now using 5 again what 
is the smallest for `x_i`\ ? this will occur when all other `x` are at 
their maximum, but we have now calculated those maxima `t_0 ... t_N`. So:

  8.  `smallest( x_i ) = (Ao - (t_0 I_0 ...` [miss out `t_i I_i`] `... t_N I_N))/I_i`

but we also know that `xI ≥ 0` so this gives us the bottom limit on 
`x_i` which we call `b_i`

  9.  `b_i = maximum(0, smallest( x_i ))`

We have now established for a test value `A`, a restricted range of 
indexes which we should test for each dimension to see whether `A` is 
in `P`.
We can choose one arbitrary dimension, then iterating over all 
possible values from `b_i` to `t_i` for all the other dimensions, 
rearranging (7) and using the modulus test for integral factors our 
test becomes:

  ``(Ao - (x[0] * i[0] .. [miss Ith term] .. x[N] * i[N])) % I[i] == 0``

repeated for all possible `x_i : b_i ≤ x_i ≤ t_i`

The process of calculating `b_i` and `t_i` for each dimension may seem onerous
but in realistic arrays it hugely reduces the search space. However, 
it is possible to optimize further:

If the array is non-overlapping, the value of `b_i` and `t_i` come out the 
identical for the outermost dimension (the one with the largest 
increment `I_i`) incdicating that this dimension need not be iterated 
at all. This must be so because in a non-overlapping array there is 
only one possible span of the lesser dimensions within which `Ao` 
could occur. Having established this value, we now know that we can 
fix it and update the calculations for the remaining dimensions but 
with the fixed value for the outermost. If the array is overlapping, 
we may find the outermost dimension does not reduce to a single span 
to be searched, but nevertheless, substituting the values for `t_i` and 
`b_i` into the equations above reduces the range to search within other 
dimensions further.

Generalising this principle, if the dimensions are sorted so they 
are tested in order from outermost (largest increment) to innermost 
(smallest increment), instead of the declaration order which is 
arbitrary, the search reduces to the smallest possible range.

