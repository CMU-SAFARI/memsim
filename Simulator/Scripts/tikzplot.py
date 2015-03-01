# ------------------------------------------------------------------------------
# File: tikzplot.py
#
# This file defines functions that generates the tikz script corresponding to
# the required plot. Provides high level constructs.
# ------------------------------------------------------------------------------

import sys


# ------------------------------------------------------------------------------
# TikZPlot class
# ------------------------------------------------------------------------------

class TikZPlot:

    
    # --------------------------------------------------------------------------
    # Initialization function
    # --------------------------------------------------------------------------

    def __init__(self):
        
        # ----------------------------------------------------------------------
        # Initialize all the internal variables
        # ----------------------------------------------------------------------

        self._styles = {}
        self._style_order = []
        self._script = []
        self._subpicture_depth = 0
        self._scale = 1


    # --------------------------------------------------------------------------
    # Function to set scale
    # --------------------------------------------------------------------------

    def set_scale(self, scale):
        self._scale = scale

    # --------------------------------------------------------------------------
    # Function to create new styles
    # --------------------------------------------------------------------------

    def style(self, name, options):

        # ----------------------------------------------------------------------
        # Check if the style is already added
        # ----------------------------------------------------------------------
        
        if name in self._styles:
            print "Error: style named `" + name + "' already added"
            quit()

        
        # ----------------------------------------------------------------------
        # Add the style to the list
        # ----------------------------------------------------------------------

        self._styles[name] = options
        self._style_order.append(name)


    # --------------------------------------------------------------------------
    # Function to add text to the plot
    # --------------------------------------------------------------------------

    def text(self, coord, text, fontsize = "normalsize", options = ""):

        # ----------------------------------------------------------------------
        # Add the text to the script
        # ----------------------------------------------------------------------

        command = "\\node " + options + " at " + self._coord(coord) + " {" + \
            "\\" + fontsize + " " + text + "};"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to draw a line
    # --------------------------------------------------------------------------

    def line(self, coord1, coord2, options = ""):

        # ----------------------------------------------------------------------
        # Add the line to the script
        # ----------------------------------------------------------------------
        
        command = "\\draw " + options + " " + self._coord(coord1) + " -- " + \
            self._coord(coord2) + ";"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to draw a line with some shift
    # --------------------------------------------------------------------------

    def line_shift(self, coord, xshift, yshift, options = ""):

        # ----------------------------------------------------------------------
        # Add the line to the script
        # ----------------------------------------------------------------------

        command = "\\draw " + options + " " + self._coord(coord) + " -- " + \
            self._shift((xshift, yshift)) + ";"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to draw a rectangle
    # --------------------------------------------------------------------------
    
    def rectangle(self, coord, width, height, options = ""):

        # ----------------------------------------------------------------------
        # Add the rectangle to the script
        # ----------------------------------------------------------------------

        command = "\\draw " + options + " " + self._coord(coord) + \
            " rectangle " + self._shift((width, height)) + ";"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to add a comment
    # --------------------------------------------------------------------------

    def comment(self, text):

        # ----------------------------------------------------------------------
        # Add comment to the script
        # ----------------------------------------------------------------------

        command = "% " + text
        self._script.append("")
        self._script.append("% " + "-" * 78)
        self._script.append(command)
        self._script.append("% " + "-" * 78)
        self._script.append("")


    # --------------------------------------------------------------------------
    # Function to begin a sub-picture
    # --------------------------------------------------------------------------
    
    def begin_subpicture(self, coord, options):

        self._subpicture_depth = self._subpicture_depth + 1
        command = "\\node " + options + " [inner sep=2pt] at " + self._coord(coord) + "{ " + \
            " \\begin{tikzpicture}"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to end a sub-picture
    # --------------------------------------------------------------------------
    
    def end_subpicture(self):

        self._subpicture_depth = self._subpicture_depth - 1
        if self._subpicture_depth < 0:
            print "Error: No subpicture to end"
            quit()

        command = "\\end{tikzpicture} };"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to add empty space (for clarity)
    # --------------------------------------------------------------------------
        
    def space(self):

        # ----------------------------------------------------------------------
        # Add couple of empty lines to the script
        # ----------------------------------------------------------------------

        self._script.append("")


    # --------------------------------------------------------------------------
    # Function to draw a path
    # --------------------------------------------------------------------------
    
    def path(self, coords, mark = None, options = ""):

        if mark is None:
            sequence = " -- ".join([self._coord(coord) for coord in coords])
            command = "\\draw " + options + " " + sequence + ";"
        else:
            sequence = " -- ".join(["%s circle(%gpt)" % (self._coord(coord), mark) for coord in coords])
            command = "\\draw [fill=black] " + options + " " + sequence + ";"
        self._script.append(command)


    # --------------------------------------------------------------------------
    # Function to draw a set of bars with equal width
    # --------------------------------------------------------------------------

    def bar_set(self, lefts, bottoms, heights, width, options = ""):

        # ----------------------------------------------------------------------
        # For each bar, call the corresponding rectangle function
        # ----------------------------------------------------------------------

        for left, bottom, height in zip(lefts, bottoms, heights):
            self.rectangle((left, bottom), width, height, options)


    # --------------------------------------------------------------------------
    # Function to create the script and output it to file
    # --------------------------------------------------------------------------

    def emit_script(self, filename = "", options = ""):

        # ----------------------------------------------------------------------
        # Check if the subpictures are all ended
        # ----------------------------------------------------------------------

        if self._subpicture_depth > 0:
            print "Error: Not all subpictures have ended"
            quit()

        # ----------------------------------------------------------------------
        # Open a file if the filename is provided
        # ----------------------------------------------------------------------

        if len(filename) > 0:
            if len(options) == 0:
                options = "w"
            fout = open(filename, options)
        else:
            fout = sys.stdout

        # ----------------------------------------------------------------------
        # Dump the script into the file
        # ----------------------------------------------------------------------

        fout.write("\\begin{tikzpicture}[scale=" + str(self._scale) + "]\n\n")

        # ----------------------------------------------------------------------
        # Styles
        # ----------------------------------------------------------------------

        fout.write("% " + "-" * 78 + "\n")
        fout.write("% List of available styles\n")
        fout.write("% " + "-" * 78 + "\n")
        for name in self._style_order:
            style_options = self._styles[name]
            command = "\\tikzset{" + name + "/.style={" + style_options + "}};\n"
            fout.write(command)
        fout.write("\n")

        # ----------------------------------------------------------------------
        # Script
        # ----------------------------------------------------------------------

        fout.writelines([line + "\n" for line in self._script])

        # ----------------------------------------------------------------------
        # End
        # ----------------------------------------------------------------------

        fout.write("\n\\end{tikzpicture}\n")

        if len(filename) > 0:
            fout.close()


    # --------------------------------------------------------------------------
    # Internal function to generate a coordinate
    # --------------------------------------------------------------------------

    def _coord(self, coord):
        
        (x, y) = coord
        return "(" + str(x) + "mm, " + str(y) + "mm)"


    # --------------------------------------------------------------------------
    # Internal function to generate a shift
    # --------------------------------------------------------------------------

    def _shift(self, shift):
        
        (xshift, yshift) = shift
        return "++(" + str(xshift) + "mm, " + str(yshift) + "mm)"
