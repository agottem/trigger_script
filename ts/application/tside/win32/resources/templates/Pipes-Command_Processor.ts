# new lines written to this file will trigger the
# listen action below.  commands.txt must exist
# in the working directory.  Alternatively, provide
# a fully qualified path such as "C:\\commands.txt"
pipe("commands.txt")

# Configure a graph to plot random values to
graph("Random vs time")
set_graph_color("DDDDDD")
set_plot_color("FF0000")
set_plot_mode("lines")


# Wait for an action to be supplied to commands.txt.
# If "stop\n" is supplied, the script will stop
# If "clear\n" is supplied, the graph will be cleared
# If "plot <x> <y>\n" is supplied, a new point will be added
action listen_pipe()

    if read_pipe(0) == "stop"
        finish
    elseif read_pipe(0) == "clear"
        erase_plot()
    elseif read_pipe(0) == "plot"
        plot(read_pipe(1), read_pipe(2))
    end

end

