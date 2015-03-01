# ------------------------------------------------------------------------------
# File: kvdata.py
# 
# This script parses multiple key-value files. It can be used to perform certain
# operations on the data like normalizing values of one file with respect to
# another, computing the geometric mean, arithmetic mean, etc.
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Import a couple of math functions
# ------------------------------------------------------------------------------

from math import exp,log


# ------------------------------------------------------------------------------
# Main class
# ------------------------------------------------------------------------------

class KeyValueData:
    
    # --------------------------------------------------------------------------
    # Initialization function. 
    # --------------------------------------------------------------------------

    def __init__(self):
        self._sets = {}
        self._keys = []


    # --------------------------------------------------------------------------
    # Function to read an input file and extract the key-value pairs from the
    # input file. The data is converted into a dictionary. It is added to the
    # global dictionary containing data from all the files.
    # --------------------------------------------------------------------------

    def read_file(self, name, filepath):
        
        # check if the name is already present in the set
        if name in self._sets:
            print "Cannot read file into set"
            print "Set with name `" + name + "' already present."
            quit()

        # open the file
        fin = open(filepath, "r")
        if not fin:
            print "Cannot open file `" + filepath + "' for reading."
            quit()

        # for each line in file, read the key value pair
        data = {}
        for line in fin:
            fields = line.split()
            key = fields[0]
            value = float(fields[1])
            data[key] = value

        # insert the data into sets
        self._sets[name] = data


    # --------------------------------------------------------------------------
    # Function to directly read a key-value dictionary
    # --------------------------------------------------------------------------

    def add_data(self, name, data):
        
        # check if the name is already present in the set
        if name in self._sets:
            print "Cannot read file into set"
            print "Set with name `" + name + "' already present."
            quit()
        
        self._sets[name] = data


    # --------------------------------------------------------------------------
    # Function to extract keys that are present in all the sets
    # --------------------------------------------------------------------------

    def extract_keys(self):
        
        # get the list of names
        names = self._sets.keys()

        # initialize available keys to the ones in the first set
        self._keys = self._sets[names[0]].keys()

        # for the remaining names, check if there is a key that is not present
        for name in names[1:]:
            self._keys = [key for key in self._sets[name].keys()\
                if key in self._keys]

        self._keys = sorted(self._keys)

        # remove unwanted keys from the sets
        for name, data in self._sets.iteritems():
            for key in data.copy().keys():
                if not key in self._keys:
                    del data[key]


    # --------------------------------------------------------------------------
    # Function to normalize the values of one set of data with respect to
    # another. The baseline is indicated by its name.
    # --------------------------------------------------------------------------

    def normalize(self, baseline):

        # ensure that the baseline is in set
        if not baseline in self._sets:
            print "Specified baseline `" + baseline + "' not present."
            quit()

        # create a copy of the baseline
        basedata = self._sets[baseline].copy()

        # for each data set, for each key in available keys, normalize
        for name, data in self._sets.iteritems():
            for key in self._keys:
                if basedata[key] == 0:
                    print "Denominator 0 for key " + key
                    quit()
                data[key] = data[key]/basedata[key]

	

    # --------------------------------------------------------------------------
    # Function to append the arithmetic mean for the data. The arithmetic mean
    # is added with the key "amean". An optional different key can be provided.
    # --------------------------------------------------------------------------

    def append_amean(self, keyname = "amean"):

        # for each data set, compute arithmetic mean and add it
        for name, data in self._sets.iteritems():
            amean = sum(data.values())/len(data.values())
            data[keyname] = amean
        self._keys.append(keyname)


    # --------------------------------------------------------------------------
    # Function to append the geometric mean for the data. Similar to amean
    # --------------------------------------------------------------------------

    def append_gmean(self, keyname = "gmean"):

        # for each data set, compute the geometric mean and add it
        for name, data in self._sets.iteritems():
            gmean = exp(sum([log(x) for x in data.values()])/len(data.values()))
            data[keyname] = gmean
        self._keys.append(keyname)


    # --------------------------------------------------------------------------
    # Function to get the keys
    # --------------------------------------------------------------------------

    def keys(self):
        return self._keys

    # --------------------------------------------------------------------------
    # Function to get the data corresponding to a particular set
    # --------------------------------------------------------------------------

    def values(self, name, key_order = None):

        # ensure name is in the sets
        if not name in self._sets:
            print "Specified data set `" + name + "' not present"
            quit()

        if key_order is None:
            ret_order = self._keys
        else:
            ret_order = key_order

        # return a copy of the data set
        return [self._sets[name][key] for key in ret_order]


    # --------------------------------------------------------------------------
    # Function to return data in key-value format
    # --------------------------------------------------------------------------

    def get_kvdata(self, name):

        # ensure name is in the sets
        if not name in self._sets:
            print "Specified data set `" + name + "' not present"
            quit()

        # return a copy of the data set
        return self._sets[name].copy()

