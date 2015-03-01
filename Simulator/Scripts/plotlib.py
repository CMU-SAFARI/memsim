# ------------------------------------------------------------------------------
# File: plotlib.py
#
# This file defines a generic interface to the plotting. Internally things are
# implemented using the tikz plot library. 
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Import necessary libraries
# ------------------------------------------------------------------------------

import sys
import os
from tikzplot import TikZPlot


# ------------------------------------------------------------------------------
# Main plot library class
# ------------------------------------------------------------------------------

class PlotLib:

    # --------------------------------------------------------------------------
    # Initialization function. Sets the default values for several plot related
    # variables.
    # --------------------------------------------------------------------------

    def __init__(self):
        
        # ----------------------------------------------------------------------
        # TikZ object
        # ----------------------------------------------------------------------

        self._tikz = TikZPlot()

        # ----------------------------------------------------------------------
        # Plot layout related variables. All values are in mm.
        # ----------------------------------------------------------------------

        self._top_margin = 10
        self._bottom_margin = 10
        self._left_margin = 10
        self._right_margin = 10

        self._plot_width = 200
        self._plot_height = 100
        self._tikzscale = 1;

        self._ceiling = 0.8

        self._y_tics_step = 0
        self._num_y_tics = 0
        self._y_tics_size = 2
        self._y_tics_fontsize = "small"
        self._y_tics_shift = -3
        self._y_tics_width = 20

        self._x_tics_shift = -1
        self._x_tics_fontsize = "small"
        self._x_tics_height = 20
        self._x_tics_size = 2

        self._title_xshift = 0
        self._title_yshift = 10
        self._title_fontsize = "normalsize"

        self._x_label_xshift = 0
        self._x_label_yshift = -15
        self._x_label_fontsize = "normalsize"
        self._x_label_height = 0

        self._y_label_xshift = -15
        self._y_label_yshift = 0
        self._y_label_fontsize = "normalsize"
        self._y_label_width = 10

        self._legend_x = self._plot_width - 20
        self._legend_y = self._plot_height - 5
        self._legend_xshift = 0
        self._legend_yshift = 0
        self._legend_entry_xshift = 1
        self._legend_entry_yshift = 1
        self._legend_entry_height = 4
        self._legend_text_xshift = 3
        self._legend_text_yshift = 1
        self._legend_box_width = 2
        self._legend_box_height = 2
        self._legend_fontsize = "scriptsize"

        # ----------------------------------------------------------------------
        # Colors and patterns
        # ----------------------------------------------------------------------
        
        self._line_styles = ['black!40,semithick', 
                             'densely dotted,thick', 
                             'dashed,thick', 
                             'black,thick',
                             'black!60,semithick', 
                             'densely dashed,thick', 
                             'red',
                             'blue',
                             ]

        self._bar_styles = ['fill=black!60',
                            'fill=black!100',
                            'fill=black!0',
                            'pattern=north west lines',
                            'fill=black!40',
                            'pattern=north east lines',
                            'pattern=crosshatch',
                            'pattern=crosshatch dots',
                            'fill=black!20',
                            'pattern=horizontal lines',
                            'fill=black!80']

        self._bar_styles2 = ['fill=black!30',
                            'fill=black!100',
                            ]

        self._bar_styles3 = ['fill=black!0',
                            'pattern=crosshatch',
                            'fill=black!100',
                            ]


    # --------------------------------------------------------------------------
    # Function to add title
    # --------------------------------------------------------------------------

    def title(self, title, xshift = 0, yshift = 0, fontsize = "", options = ""):

        if len(fontsize) == 0:
            fontsize = self._title_fontsize

        # ----------------------------------------------------------------------
        # Compute the title x and y coords
        # ----------------------------------------------------------------------
        
        title_x = self._plot_width / 2 + self._title_xshift + xshift
        title_y = self._plot_height + self._title_yshift + yshift
        self._tikz.comment("Plot title")
        self._tikz.text((title_x, title_y), title, \
            fontsize = fontsize, options = options)


    # --------------------------------------------------------------------------
    # Function to add xlabel
    # --------------------------------------------------------------------------

    def xlabel(self, xlabel, xshift = 0, yshift = 0, fontsize = "", options = ""):

        if len(fontsize) == 0:
            fontsize = self._x_label_fontsize

        # ----------------------------------------------------------------------
        # Compute the xlabel x and y coords
        # ----------------------------------------------------------------------
        
        x_label_x = self._plot_width / 2 + self._x_label_xshift + xshift
        x_label_y = self._x_label_yshift + yshift
        self._tikz.comment("Plot x-label")
        self._tikz.text((x_label_x, x_label_y), xlabel, \
            fontsize = fontsize, options = options)


    # --------------------------------------------------------------------------
    # Function to add xlabel
    # --------------------------------------------------------------------------

    def ylabel(self, ylabel, xshift = 0, yshift = 0, fontsize = "", options = ""):

        if len(fontsize) == 0:
            fontsize = self._y_label_fontsize

        # ----------------------------------------------------------------------
        # Compute the ylabel x and y coords
        # ----------------------------------------------------------------------
        
        y_label_x = self._y_label_xshift + xshift
        y_label_y = self._plot_height / 2 + self._y_label_yshift + yshift
        self._tikz.comment("Plot y-label")
        self._tikz.text((y_label_x, y_label_y), ylabel, \
            fontsize = fontsize, options = "[rotate=90] " + options)


    # --------------------------------------------------------------------------
    # Function to plot column stacked bars
    # --------------------------------------------------------------------------

    def column_stacked_bars(self, keys, sets, legend = [], colors = [],
        patterns = [], value_base = 0, value_ceiling = 0,
        key_padding = 0.3):

        # ----------------------------------------------------------------------
        # Ensure everything is correct
        # ----------------------------------------------------------------------

        num_error = sum([1 for values in sets if len(values) != len(keys)])
        if num_error > 0:
            print "Error: Mismatch in number of values for " + \
                str(num_error) + " set(s)"
            quit()

        set_names = [""] * len(sets)
        if len(legend) > 0 and len(legend) != len(sets):
            print "Error: Mismatch in the number of legends specified"
            quit()
        elif len(legend) > 0:
            set_names = legend[:]

        # ----------------------------------------------------------------------
        # Add the colors and patterns if they are specified and compute styles
        # ----------------------------------------------------------------------


        if len(self._bar_styles) < len(sets):
            print "Not enough styles available"
            quit()

        if len(sets) == 2:
            for index, style in enumerate(self._bar_styles2):
                name = "bar-style-" + str(index + 1)
                self._tikz.style(name, style)
        elif len(sets) == 3:
            for index, style in enumerate(self._bar_styles3):
                name = "bar-style-" + str(index + 1)
                self._tikz.style(name, style)
        else:
            for index, style in enumerate(self._bar_styles):
                name = "bar-style-" + str(index + 1)
                self._tikz.style(name, style)
        
        styles = ["bar-style-" + str(i + 1) for i in range(len(sets))]

        

        # ----------------------------------------------------------------------
        # Compute the width of the bars
        # ----------------------------------------------------------------------

        key_width = float(self._plot_width) / len(keys)
        bar_width = float(key_width) * (1 - key_padding) / len(sets)

        # ----------------------------------------------------------------------
        # Compute the x-location for each bar
        # ----------------------------------------------------------------------
        
        key_left_padding = key_padding * key_width / 2.0
        x_values = [[j * key_width + key_left_padding + i * bar_width \
            for j in range(len(keys))] for i in range(len(sets))]

        # ----------------------------------------------------------------------
        # Compute the plot values corresponding to the data and y-tics
        # ----------------------------------------------------------------------

        (y_values, y_tics) = self._compute_plot_values(sets, value_base, \
            value_ceiling)

        # ----------------------------------------------------------------------
        # Bounding box
        # ----------------------------------------------------------------------

        self._tikz.rectangle((0, 0), self._plot_width, self._plot_height)
        self._tikz.space()
        
        # ----------------------------------------------------------------------
        # Plot the y-tics
        # ----------------------------------------------------------------------
        
        self._plot_y_tics(y_tics)
        self._tikz.space()

        # ----------------------------------------------------------------------
        # Plot the bars with the corresponding styles
        # ----------------------------------------------------------------------
 
        bottom = [0] * len(keys)
        for lefts, heights, style, name in zip(x_values, y_values, styles,
                set_names):
            self._tikz.comment("WHITE BACKGROUND : " + name)
            self._tikz.bar_set(lefts, bottom, heights, bar_width,
                options = "[fill=white]")
            self._tikz.space()
            self._tikz.comment("REAL STYLE : " + name)
            self._tikz.bar_set(lefts, bottom, heights, bar_width, 
                options = "[style=" + style + "]")
            self._tikz.space()

        # ----------------------------------------------------------------------
        # Plot the keys
        # ----------------------------------------------------------------------
        
        self._tikz.comment("X-Tics: Keys")
        for i, key in enumerate(keys):
            key_x = float(i) * key_width + key_width / 2.0
            self._tikz.text((key_x, self._x_tics_shift), key, \
                options = "[rotate=90,left]", fontsize = self._x_tics_fontsize)
        self._tikz.space()

        # ----------------------------------------------------------------------
        # Auto set x-label distance 
        # ----------------------------------------------------------------------

        max_key_len = max([len(key) for key in keys])
        self._x_label_yshift = -(max_key_len * 2.5) - 5

        # ----------------------------------------------------------------------
        # Plot the legend
        # ----------------------------------------------------------------------
    
        if len(legend) > 0:
            self._plot_legend("box", legend, styles)


    # --------------------------------------------------------------------------
    # Function to plot continuous lines
    # --------------------------------------------------------------------------

    def continuous_lines(self, lines, legend = [], colors = [], xtics = [],
        use_markers = 1, value_base = 0, value_ceiling = 0, x_padding = 15):

        set_names = [''] * len(lines)
        if len(legend) > 0:
            set_names = legend[:]

        # ----------------------------------------------------------------------
        # Add the colors to the list
        # ----------------------------------------------------------------------

        if len(self._line_styles) < len(lines):
            print "Not enough line styles"
            quit()

        for index, style in enumerate(self._line_styles):
            name = "line-style-" + str(index + 1)
            self._tikz.style(name, style)

        styles = ["line-style-" + str(i + 1) for i in range(len(lines))]


        # ----------------------------------------------------------------------
        # Split the x-values and y-values
        # ----------------------------------------------------------------------

        x_values = [[x for x, y in line] for line in lines]
        y_values = [[y for x, y in line] for line in lines]

        # ----------------------------------------------------------------------
        # Compute the plot values
        # ----------------------------------------------------------------------

        (y_plot_values, y_tics) = self._compute_plot_values(y_values, \
            value_base, value_ceiling)

        # ----------------------------------------------------------------------
        # Compute the x-scaling
        # ----------------------------------------------------------------------

        x_min = self._find_min(x_values)
        x_max = self._find_max(x_values)
        available_width = float(self._plot_width) - x_padding * 2
        x_scaling = available_width / (x_max - x_min)
        x_plot_values = self._scale(x_values, x_scaling, x_min)
        x_plot_values = [[x + x_padding for x in xlist] for xlist in\
            x_plot_values]
        x_tic_values = self._scale(xtics, x_scaling, x_min)
        x_tic_values = [x + x_padding for x in x_tic_values]

        # ----------------------------------------------------------------------
        # Bounding box
        # ----------------------------------------------------------------------

        self._tikz.rectangle((0,0), self._plot_width, self._plot_height)

        # ----------------------------------------------------------------------
        # Plot y-tics
        # ----------------------------------------------------------------------

        self._plot_y_tics(y_tics)

        # ----------------------------------------------------------------------
        # Plot x-tics
        # ----------------------------------------------------------------------

        self._plot_x_tics(zip(xtics, x_tic_values))

        # ----------------------------------------------------------------------
        # Plot the lines
        # ----------------------------------------------------------------------

        paths = [zip(xlist, ylist) for xlist, ylist in zip(x_plot_values,\
            y_plot_values)]

        for i, path in enumerate(paths):
            self._tikz.comment("Line: " + set_names[i])
            options = "[" + styles[i] + "]"
            self._tikz.path(path, mark = use_markers, options = options)
            self._tikz.space()


        # ----------------------------------------------------------------------
        # Plot the legend
        # ----------------------------------------------------------------------

        if len(legend) > 0:
            self._plot_legend("line", legend, styles)


    # --------------------------------------------------------------------------
    # Function to output tikz
    # --------------------------------------------------------------------------

    def save_tikz(self, filename = ""):
        
        # ----------------------------------------------------------------------
        # Just pass the filename to emit script
        # ----------------------------------------------------------------------
 
        self._tikz.emit_script(filename)


    # --------------------------------------------------------------------------
    # Function to output tex
    # --------------------------------------------------------------------------

    def save_tex(self, filename = ""):

        # ----------------------------------------------------------------------
        # Check if the filename is given
        # ----------------------------------------------------------------------

        if len(filename) > 0:
            filename = filename + ".tex"
            fout = open(filename, "w")
        else:
            fout = sys.stdout

        # ----------------------------------------------------------------------
        # Compute paper width and height
        # ----------------------------------------------------------------------
        
        paper_width = self._plot_width + self._left_margin + \
            self._right_margin - self._y_label_xshift + self._y_label_width
        paper_height = self._plot_height + self._top_margin + \
            self._title_yshift * 2 - self._x_label_yshift + \
            self._x_label_height + self._bottom_margin
    
        # ----------------------------------------------------------------------
        # Output TeX header to the file
        # ----------------------------------------------------------------------
        
        tex_header = """
        \\documentclass[12pt,dvipsnames]{article}

        \\usepackage{tikz}
        \\usetikzlibrary{patterns}
        \\usetikzlibrary{calc}

        \\usepackage{fullpage}
        \\usepackage{color}
        """
        tex_header = tex_header + "\\usepackage[top=" + \
            str(self._top_margin) + "mm,left=" + str(self._left_margin) + \
            "mm,right=" + str(self._right_margin) + "mm,bottom=" + \
            str(self._bottom_margin) + "mm]{geometry}\n" + \
            "\\setlength{\\paperwidth}{" + str(paper_width) + "mm}\n" + \
            "\\setlength{\\paperheight}{" + str(paper_height) + "mm}\n" + \
            "\\begin{document}\n"
        
        fout.write(tex_header)
        if len(filename) > 0:
            fout.close()
        
        self._tikz.emit_script(filename, "a")

        if len(filename) > 0:
            fout = open(filename, "a")
        fout.write("\\end{document}\n")
        fout.close()


    # --------------------------------------------------------------------------
    # Function to save the file as ps
    # --------------------------------------------------------------------------

    def save_ps(self, filename = "a"):

        # ----------------------------------------------------------------------
        # Save tex
        # ----------------------------------------------------------------------

        self.save_tex(filename)

        # ----------------------------------------------------------------------
        # Compile tex to dvi and then to ps
        # ----------------------------------------------------------------------

        ret = os.system("latex -halt-on-error " + filename)
        if ret != 0:
            print "Latex encountered some errors"
            quit()

        ret = os.system("dvips -q " + filename + ".dvi")
        if ret != 0:
            print "dvips encountered some errors"
            quit()

        # ----------------------------------------------------------------------
        # Clean up
        # ----------------------------------------------------------------------

        os.remove(filename + ".aux")
        os.remove(filename + ".log")
        os.remove(filename + ".dvi")


    # --------------------------------------------------------------------------
    # Function to save pdf
    # --------------------------------------------------------------------------

    def save_pdf(self, filename = "a"):

        # ----------------------------------------------------------------------
        # Save ps and use ps2pdf 
        # ----------------------------------------------------------------------

        self.save_ps(filename)
        ret = os.system("ps2pdf " + filename + ".ps")
        if ret != 0:
            print "ps2pdf returned some errors"
            quit()

        # ----------------------------------------------------------------------
        # Clean up
        # ----------------------------------------------------------------------

        os.remove(filename + ".ps")


    # --------------------------------------------------------------------------
    # Function to plot y-tics
    # --------------------------------------------------------------------------
    
    def _plot_y_tics(self, y_tics):

        self._tikz.comment("Y-Tics")
        for value, y in y_tics:
            self._tikz.line_shift((0,y), -self._y_tics_size, 0)
            self._tikz.line_shift((0,y), self._plot_width, 0, \
                options = "[thin,densely dotted]")
            self._tikz.text((self._y_tics_shift,y), str(value), \
                options = "[left]", fontsize = self._y_tics_fontsize)
            
        self._tikz.space()


    # --------------------------------------------------------------------------
    # Function to plot y-tics
    # --------------------------------------------------------------------------

    def _plot_x_tics(self, x_tics):
        
        self._tikz.comment("X-Tics")
        for value, x in x_tics:
            self._tikz.line_shift((x,0), 0, -2)
            self._tikz.text((x, self._x_tics_shift -3), str(value), \
               fontsize = self._x_tics_fontsize)
            
        self._tikz.space()


    # --------------------------------------------------------------------------
    # Function to plot legend
    # --------------------------------------------------------------------------

    def _plot_legend(self, legend_type, legend, styles):

        if legend_type == "line":
            self._legend_text_xshift = self._legend_text_xshift + \
                self._legend_box_width

        self._tikz.comment("Legend for the keys")

        # ----------------------------------------------------------------------
        # Start the sub picture for the legend and legend boundary
        # ----------------------------------------------------------------------
        
        self._tikz.begin_subpicture((self._legend_x + self._legend_xshift, \
            self._legend_y + self._legend_yshift), options = "[fill=white,draw]")
        self._tikz.text((0,0), "")
        
        # ----------------------------------------------------------------------
        # One line per legend
        # ----------------------------------------------------------------------
        
        for i, (text, style) in enumerate(reversed(zip(legend, styles))):
            legend_i_x = self._legend_entry_xshift
            legend_i_y = self._legend_entry_yshift + \
                i * self._legend_entry_height
            
            if legend_type == "box":
                self._tikz.rectangle((legend_i_x, legend_i_y), \
                    self._legend_box_width, self._legend_box_height, \
                    options = "[style=" + style + "]")
            elif legend_type == "line":
                self._tikz.line_shift((legend_i_x, legend_i_y +\
                self._legend_box_height/2), \
                    self._legend_box_width*2, 0, options = "[" + style + "]")

            self._tikz.text((legend_i_x + self._legend_text_xshift, \
                legend_i_y + self._legend_text_yshift), text, \
                fontsize = self._legend_fontsize, options = "[right]")

        self._tikz.end_subpicture()


    # --------------------------------------------------------------------------
    # Function to compute the plot values 
    # --------------------------------------------------------------------------

    def _compute_plot_values(self, values, value_base, value_ceiling):

        # ----------------------------------------------------------------------
        # Find the minimum and maximum value
        # ----------------------------------------------------------------------

        value_min = min(value_base, self._find_min(values))
        value_max = max(value_ceiling, self._find_max(values))

        # ----------------------------------------------------------------------
        # Find the scaling factor
        # ----------------------------------------------------------------------

        available_height = self._plot_height * self._ceiling
        scaling_factor = available_height / (value_max - value_min)
        print scaling_factor

        # ----------------------------------------------------------------------
        # Scale the values
        # ----------------------------------------------------------------------

        y_vals = self._scale(values, scaling_factor, value_min)

        # ----------------------------------------------------------------------
        # Compute y-tics
        # ----------------------------------------------------------------------

        if self._y_tics_step == 0 and self._num_y_tics == 0:
            self._num_y_tics = 10

        if self._y_tics_step == 0:
            self._y_tics_step = (value_max - value_min) / \
                (self._num_y_tics * self._ceiling)
            self._y_tics_step = round(self._y_tics_step, 2)
        elif self._num_y_tics == 0:
            self._num_y_tics = 1 + int(self._plot_height / \
                (scaling_factor * self._y_tics_step))


        y_tics = [(value_min + i * self._y_tics_step, \
            self._scale(i * self._y_tics_step, scaling_factor, 0)) \
            for i in range(1, self._num_y_tics)]

        return (y_vals, y_tics)


    # --------------------------------------------------------------------------
    # Compute the minimum value
    # --------------------------------------------------------------------------

    def _find_min(self, values):
        if type(values) is not list:
            return values
        else:
            return min([self._find_min(value) for value in values])


    # --------------------------------------------------------------------------
    # Compute the maximum value
    # --------------------------------------------------------------------------

    def _find_max(self, values):
        if type(values) is not list:
            return values
        else:
            return max([self._find_max(value) for value in values])


    # --------------------------------------------------------------------------
    # Scale the values
    # --------------------------------------------------------------------------

    def _scale(self, values, scaling_factor, value_base):
        if type(values) is not list:
            return (values - value_base) * scaling_factor
        else:
            return [self._scale(value, scaling_factor, value_base) \
                for value in values]


    # --------------------------------------------------------------------------
    # Function to set the y-tics step
    # --------------------------------------------------------------------------

    def set_y_tics_step(self, value):
        self._y_tics_step = value

    # --------------------------------------------------------------------------
    # Function to set the y-tics shift
    # --------------------------------------------------------------------------

    def set_y_tics_shift(self, value):
        self._y_tics_shift = value

    # --------------------------------------------------------------------------
    # Function to shift the legend
    # --------------------------------------------------------------------------

    def set_legend_shift(self, xshift = 0, yshift = 0):
        self._legend_xshift = xshift
        self._legend_yshift = yshift


    # --------------------------------------------------------------------------
    # Function to set the ceiling
    # --------------------------------------------------------------------------

    def set_ceiling(self, ceiling):
        self._ceiling = ceiling

    # --------------------------------------------------------------------------
    # Function to set plot width and height
    # --------------------------------------------------------------------------
    
    def set_plot_dimensions(self, width = 0, height = 0):
        if width != 0:
            self._plot_width = width
        if height != 0:
            self._plot_height = height

    # --------------------------------------------------------------------------
    # Function to set the y-tics font size
    # --------------------------------------------------------------------------

    def set_y_tics_font_size(self, fontsize):
        self._y_tics_fontsize = fontsize

    # --------------------------------------------------------------------------
    # Function to set the x-tics font size
    # --------------------------------------------------------------------------

    def set_x_tics_font_size(self, fontsize):
        self._x_tics_fontsize = fontsize

    # --------------------------------------------------------------------------
    # Function to set the y-label font size
    # --------------------------------------------------------------------------

    def set_y_label_font_size(self, fontsize):
        self._y_label_fontsize = fontsize

    # --------------------------------------------------------------------------
    # Function to set the y-tics font size
    # --------------------------------------------------------------------------

    def set_x_label_font_size(self, fontsize):
        self._x_label_fontsize = fontsize


    # --------------------------------------------------------------------------
    # Function to set scale
    # --------------------------------------------------------------------------

    def set_scale(self, scale):
        self._tikzscale = scale
        self._tikz.set_scale(scale)
