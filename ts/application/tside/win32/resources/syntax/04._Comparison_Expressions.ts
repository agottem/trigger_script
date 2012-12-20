# Comparisons can be made between values of variables, constants, or the
# output of functions.  The result of a comparison is a boolean value
# indicating the validity of the comparison expression.  Comparisons are
# made using the following operators:

#    x > y  Tests if x is greater than y
#    x >= y Tests if x is greater than or equal to y
#    x < y  Tests if x is less than y
#    x <= y Tests if x is less than or equal to y
#    x == y Tests if x equals y
#    x ~= y Tests if x is not equal to y

# Comparison operators can be chained together to form more complicated
# comparisons.  For example, the following are valid expressions:

# result is true
result = 5 > 4 > 3 > 2 > 1

# result is true
result = 5 == 5 ~= 4 > 3 < 5

