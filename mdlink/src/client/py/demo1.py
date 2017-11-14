import jzs
import random
import pandas as pd


ts = None

def init():
    global ts
    ts = jzs.TradeSystem()
    ts.connect( "127.0.0.1", 221)

def print_bar1m():
    bar, quote = ts.bar1m("IF1508.CFE")
    print last
    print bar

def buy():

    portfolio = ts.query_portfolio()
    if portfolio is None:
        print "Query portfolio failed"
        return

    print portfolio
    
    goal = pd.DataFrame(portfolio['size'])
    goal['refpx'] = 0.0
    goal['urgency'] = 10

    #goal['refpx']['IF1508.CFE'] = ts.quote('IF1508.CFE').last
    #goal['refpx']['IF1509.CFE'] = ts.quote('IF1509.CFE').last
    #goal['size']['IF1508.CFE'] = 0 
    #goal['size']['IF1509.CFE'] = 0
    goal['refpx']['600000.SH'] = ts.quote('600000.SH').last
    goal['size']['600000.SH'] += 100
    
    ret, err_text = ts.goal_portfolio(goal)
    if ret:
        print "goal_portfolio is accepted"
    else:
        print "goal_portfolio is failed",  err_text

def subscribe():
    ret, err_symbols = ts.subscribe(["000001.SH", "000001.SX"])
    print ret, err_symbols

def basket_order():
    single_order = [ "600000.SH", ts.quote("600000.SH").last, +100, 1]
    ret, err_text = ts.basket_order(single_order)
    print ret, err_text
    
    data = { 'symbol'   : [ '510300.SH', '150315.SZ'],
             'refpx'    : [ ts.quote("510300.SH").last, ts.quote("150315.SZ").last ],
             'inc_size' : [ -100, -200],
             'urgency'  : [ 1, 2 ] }
    orders = pd.DataFrame(data)
    ret, err_text = ts.basket_order(orders)
    print ret, err_text

def stop_portfolio():
    ret, err_text = ts.stop_portfolio()
    print ret, err_text
    
if __name__ == "__main__" :
    init()
    
    #print_bar1m()
    
    #buy()

    # Get quotes by strategy id
    #print ts.universe_quotes()
    
    #basket_order()
    
    print ts.query_portfolio()
    stop_portfolio()
