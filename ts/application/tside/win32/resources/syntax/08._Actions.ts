# Actions are a key feature in Trigger Script.  Actions allow specified
# blocks of code to only be executed when certain conditions have changed.

# Actions are always at the bottom of the program.  For example:

x = 5
print("message")

# Action is at the bottom
action timer(10)
    print("timed message")
end

# A script can have multiple action handlers, and each action handler can
# have multiple triggers associated with it.  For example:

pipe("in.txt")
action timer(5000), listen_pipe()
    message("Either it's been 5 seconds, or data is on the pipe")
end

# The above action will be executed every time a new line is appended to
# 'in.txt', and, when 5 seconds have passed.  The behavior of a trigger
# associated with an action is dependent on the trigger.  Consulting the
# documentation for the trigger is recommended.  Below are some examples
# of actions which will be run for various timer triggers:

# This action is run once, after 5 seconds
action timer(5000)
    print("Hi")
end

# This action is run every 5 seconds
action ptimer(5000)
    print("Hi")
end

# The arguments passed into a trigger may also be changed whenver that
# action is run, however, the behavior of doing so is dependent upon the
# trigger being used and the documentation should be consulted.  For
# example:

x = 5000
action ptimer(x)
    x = 20
end

# Even though x is changed to 20 in the above example, the timer will
# continue to fire every 5 seconds.  However, the below example will
# update its timer value to 20 instead of 5000:

x = 5000
action vtimer(x)
    x = 20
end

# The triggers of an action will inform the Trigger Script when they are
# finished (meaning they will no longer fire).  A script continues to
# execute until all triggers are done firing.  However, a script can be
# completed prematurely by using the 'finish' statment.  For instance:

x = 0
action ptimer(5000)
    x = x+1
    if x > 5
        finish
    end
end

# The above script will complete once the action has been triggered more
# than 5 times.

