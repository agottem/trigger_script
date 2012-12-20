# Logical expressions test the truth of multiple expressions using basic
# logical operators.  If an integer or real number is used as an operand
# of a logical expression, a 0 is treated as false while every other value
# is treated as true.  If a string is used as an operand, an empty string
# is treated as false while a non-empty string is treated as true.
# Logical expressions are made up using the following operators:

#    x and y Test if x and y are true
#    x or y  Test if x or y are true
#    not x   Test if x is not true

# In a logical expression, 'not' takes the highest precedence, followed by
# 'and', with 'or' have the least precedence.  Logical expression
# operators can be chained together to form more complicated logical
# expressions.  For example, the following is a valid expressions:

# result is true
result = 5 > 0 and 4 > 1 and 3 < 9 and 2 < 4

