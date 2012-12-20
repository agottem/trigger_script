# If statements direct the flow of execution, so sections of code can be
# conditionally executed.  Below is a basic if statement:

if 5 == 5
    print("Hello World")
end

# In the above example, 5 == 5 is true, so "Hello World" will be printed.
# If statements can be composed of a series of 'elseif' keywords, or, a
# final 'else' keyword.  The below example should be self-explanatory:

# In this example, 'D' will be printed
if 5 > 5
    print("A")
elseif 5 > 8
    print("B")
elseif 9 > 8
    print("C")
else
    print("D")
end

