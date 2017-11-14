import jzs
import random
import pandas as pd
import time


ts = None

def init():
    global ts
    ts = jzs.TradeSystem()
    ts.connect( "127.0.0.1", 2)

def print_bar1m():
    bar, quote = ts.bar1m("IF1508.CFE")
    print last
    print bar

def buy( inc_pf ):
    cur_pf = ts.query_portfolio()
    if cur_pf is None:
        print "Query portfolio failed"
        return
    
    goal = pd.DataFrame(cur_pf['size'])
    goal['refpx'] = 0.0
    goal['urgency'] = 10
    print goal
    for e in inc_pf:
        code = e[0]
        inc = e[1]
        print code
        if code not in goal.index :
            print code, 'is not in portfolio'
            print cur_pf
            return

        goal['refpx'][code] = ts.quote(code).last
        goal['size'][code] += inc
    ret = ts.goal_portfolio(goal)
    print "goal_portfolio is ", "accepted " if ret  else "failed"


if __name__ == "__main__" :
    init()
    #print_bar1m()
    
    for i in xrange(0, 1000):
        #buy([['600000.SH', 100], ['000001.SZ', 100]])
        side = (i%2)*2 - 1
        buy ([['IF1508.CFE', 1 * side], ['IF1509.CFE', -1 * side]])
        time.sleep(1)
        
                   
        
    
    

