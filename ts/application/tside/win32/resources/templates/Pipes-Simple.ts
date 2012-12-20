# in.txt is in whatever the working directory is.
# Alternatively, provide an absolute path such as
# "C:\\in.txt "
pipe("in.txt")


# Listen for a new line to be input into in.txt.
# Whenever there is a new line entered, this action
# will run and print arguments 0, 1, and 2 of the new
# line.
action listen_pipe()
    print(read_pipe(0) + " " + read_pipe(1) + " " + read_pipe(2))
end

