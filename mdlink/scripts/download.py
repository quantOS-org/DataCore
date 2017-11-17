import urllib
path = 'http://www.quantos.org/downloads/basedata/'
files = ['calendar.csv','instrument.csv','market.csv']
for f in files :
    url = path + f
    print "Download %s"%f 
    urllib.urlretrieve(url, "./etc/" + f )