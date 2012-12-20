# Create two graphs
g1 = graph("Primary Graph")
g2 = graph("Secondary Graph")


# Style the primary graph accordingly
hset_graph_color(g1, "AAAAAA")
hset_plot_mode(g1, "lines")
hset_plot_color(g1, "0000FF")


# Every tenth of a second, update the primary graph
action ptimer(100)
    hplot(g1, time(), uniform_random())
end

# Every second, update the secondary graph
action ptimer(1000)
    hplot(g2, time(), uniform_random())
end

