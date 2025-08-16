def parse_input(user_input):
    tokens = user_input.split()
    if not tokens:
        return None
    cmd = tokens[0]

    if cmd == "play":
        starttime = "0"
        endtime = "0"
        d = "0"
        dd = "0"
        i = 1
        while i < len(tokens):
            token = tokens[i]
            if token == "-ss":
                if i + 1 < len(tokens):
                    starttime = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -ss flag in play command")
                    return None
            elif token == "-end":
                if i + 1 < len(tokens):
                    endtime = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -end flag in play command")
                    return None
            elif token == "-d":
                if i + 1 < len(tokens):
                    d = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -d flag in play command")
                    return None
            elif token == "-dd":
                if i + 1 < len(tokens):
                    dd = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -dd flag in play command")
                    return None
            else:
                print(f"Invalid flag '{token}' in play command")
                return None
        return f"play -ss{starttime} -end{endtime} -d{d} -dd{dd}"
    elif cmd in ["pause", "stop"]:
        if len(tokens) != 1:
            print(f"Usage: {cmd}")
            return None
        return cmd
    elif cmd == "parttest":
        rgb = "255"
        channel = "0"
        i = 1
        while i < len(tokens):
            token = tokens[i]
            if token == "-rgb":
                if i + 1 < len(tokens):
                    rgb = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -rgb flag in parttest command")
                    return None
            elif token == "-c":
                if i + 1 < len(tokens):
                    channel = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -c flag in parttest command")
                    return None
            else:
                print(f"Invalid flag '{token}' in parttest command")
                return None
        return f"parttest -rgb{rgb} -c{channel}"
    else:
        print("Not support")
        return None
