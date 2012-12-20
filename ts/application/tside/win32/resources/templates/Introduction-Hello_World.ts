# Number of times to tell the world: hello
repeat_count = 5


# Say hello every second
action ptimer(1000)

    print("Hello World!")

    # Decrement the count.  If the count is 0, finish up
    repeat_count = repeat_count-1
    if repeat_count == 0
        finish
    end

end

