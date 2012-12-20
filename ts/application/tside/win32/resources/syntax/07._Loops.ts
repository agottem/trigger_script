# Several types of loops are supported by Trigger Script.  Starting with
# the most basic, an infinite loop can be seen below:

x = 0
loop
    print("The loop has executed " + x + "times.")

    x = x+1
end

# The above loop will never end, and will print the message to infinity.
# Two flow control statements are available to control the flow of an
# executing loop: 'break' and 'continue'.  If break is used, the loop will
# end.  If continue is used, the loop will restart at the top of it.  For
# example:

x = 0
loop
    if x > 2
        break
    end

    print("The loop has executed " + x + "times.")
    x = x+1
end

print("Done")

# In the example above, the message will be printed only twice until the
# message "Done" is printed.  Similarly:

x = 0
loop
    if x < 100
        continue
    end

    print("The loop has executed " + x + "times.")
    x = x+1
end

# The above example will loop infinitely as well.  However, the first 100
# messages will not be printed (the first message to be printed will be
# "The loop has executed 100 times.").

# Since infinite loops are only sometimes useful, 'while' loops are also
# available.  A while loop will execute until its associated condition is
# false.  For example:

x = 0
while x < 5
    print("Hello")
    x = x+1
end

# The above example will print "Hello" 5 times, with x being incremented
# after each iteration of the loop.  After 5 times, x < 5 is no longer
# true, and the loop exits.

# Notice in the while loop example that x has to be incremented manually.
# This can be cumbersome, so the concept of a 'for loop' also exists in
# Trigger Script.  A for loop is constructed in the following examples:

for x = 0 to 5
    print("Hello")
end

# The above case prints "Hello" 5 times.  After each iteration of the
# loop, x is incremented by 1.  Similarly, x can be decremented by the for
# loop instead:

for x = 5 downto 0
    print("Hello")
end

# The above case prints "Hello" 5 times as well, except x starts at 5 and
# counts backwards to 0.

# Finally, the variable does not need to be assigned in a for loop.  If
# the variable already exists, something like this can be done:

x = 10
for x to 15
    print("Hello")
end

# In this case, x starts at 10, and increments up to 15.  This results in
# "Hello" being printed 5 times.

