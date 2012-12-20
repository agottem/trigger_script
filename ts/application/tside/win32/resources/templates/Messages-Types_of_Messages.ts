# Output some messages

# Just print
print("This message does not wait for the user before continuing")

# Print and block
message("This message blocks until the user presses 'OK'")

# Ask if the user wants to see the next message
answer = choice("Do you want to see the next message?")
if answer == true
    message("You're awesome!")
end

