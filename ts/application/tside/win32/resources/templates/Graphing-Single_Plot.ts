# Create a graph
graph("My Graph")

# Style it accordingly (colors are RGB format)
set_graph_color("AAAAAA")
set_plot_mode("lines")
set_plot_color("0000FF")


# Every tenth of a second, plot a new random
action ptimer(100)
    plot(time(), uniform_random())
end

