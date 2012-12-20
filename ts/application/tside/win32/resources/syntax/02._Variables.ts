# Variables can be named using any combination of letters, numbers, and
# the '_' character.  To declare a variable, simply assign a variable to a
# value.  For example:

my_var = 5

# The above line of code creates a variable named 'my_var', with a value
# of 5.  The type of this variable is automatically determined during
# compilation.  In the above example, the type will be an integer.
# Similarly, a variable can also be a real number:

my_real_var = 3.14159

# Or a variable can be a string:

my_string_variable = "Hello World"

# Or, lastly, a variable may be a boolean:

my_boolean_variable = true

# With TS, it is typically unimportant to know what the type of a variable
# is.  Once a variable is assigned a value, it can be referenced in
# expressions and assigned to other variables.  For example:

my_var = 5
my_other_var = my_var + 1

# Variables follow normal scoping rules.  A variable declared in a block,
# will only be available in that block and any nested blocks.

