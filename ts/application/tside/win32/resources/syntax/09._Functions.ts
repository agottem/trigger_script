# Trigger Script supports defining functions.  Functions are defined by
# simply creating a new file.  For instance, imagine two files exist:

# File 1, named: foo.ts
print("this is foo.ts")

# File 2, named: main.ts
foo()

# In the above example, when file 2 ('main.ts') is run, it invokes a
# function by the name of 'foo'.  This function is identified by its file
# name, and will be called by the script.  The result of running 'main.ts'
# will be the printed message "this is foo.ts".

# Functions can define inputs and outputs.  Each function can have
# multiple inputs, but only a single output.  Outputs must be assigned a
# value, and both inputs and outputs are always declared at the top of the
# file.  For example:

# File 1, named: foo.ts
input x, y, z
output my_out = 0

my_out = x + y + z

# File 2, named: main.ts
result = foo(1, 2, 3)

print(result)

# In the above example, a function is defined by creating a new file named
# 'foo.ts'.  This function takes 3 arguments: x, y, and z.  An output is
# defined, and is initialized to 0.  When the function runs, the output
# variable 'my_out' is assigned the sum of the inputs.  The function 'foo'
# returns when the end of the file is reached.  In the above example, the
# value 6 will be printed.

# If a function has action handlers defined, the function will always
# return before any action handlers are run.  The action handlers will
# continue to run, despite the function having returned.  For example:

# File 1, named: foo.ts
output var = 0

var = uniform_random()

action ptimer(5000)
    print("I'm still running")

    var = 10
end

# File 2, named: main.ts
result = foo()

print(result)

# In the above example, foo() will return a uniform random variable,
# however, the message "I'm still running" will continue to print every 5
# seconds even though the function has returned the result to its caller.
# If foo() were called multiple times, each invocation would receive it's
# own set of action handlers each firing every 5 seconds.

