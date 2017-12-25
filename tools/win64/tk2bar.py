from tkreader import TkReader
import pandas as pd
import numpy as np
import datetime as dt
import traceback
from test._mock_backport import inplace

bar_fields = ['date', 'trade_date', 'time', 'volume_inc', 'turnover_inc', 'settle', 'oi', 
              'open', 'high', 'low', 'close', 'volume', 'turnover',
              'askprice1', 'askprice2', 'askprice3', 'askprice4', 'askprice5',
              'bidprice1', 'bidprice2', 'bidprice3', 'bidprice4', 'bidprice5',
              'askvolume1', 'askvolume2', 'askvolume3', 'askvolume4', 'askvolume5',
              'bidvolume1', 'bidvolume2', 'bidvolume3', 'bidvolume4', 'bidvolume5']
integer_fields = ['date', 'trade_date', 'time', 'volume', 'turnover', 'settle', 'oi', 
              'open', 'high', 'low', 'close', 'volume_total', 'turnover_total',
              'askprice1', 'askprice2', 'askprice3', 'askprice4', 'askprice5',
              'bidprice1', 'bidprice2', 'bidprice3', 'bidprice4', 'bidprice5',
              'askvolume1', 'askvolume2', 'askvolume3', 'askvolume4', 'askvolume5',
              'bidvolume1', 'bidvolume2', 'bidvolume3', 'bidvolume4', 'bidvolume5']

update_fields = ['trade_date', 'settle', 'oi', 
                'volume', 'turnover',
              'askprice1', 'askprice2', 'askprice3', 'askprice4', 'askprice5',
              'bidprice1', 'bidprice2', 'bidprice3', 'bidprice4', 'bidprice5',
              'askvolume1', 'askvolume2', 'askvolume3', 'askvolume4', 'askvolume5',
              'bidvolume1', 'bidvolume2', 'bidvolume3', 'bidvolume4', 'bidvolume5']

def mstime2daymillis(time):
    ms  = time % 1000
    tmp = time / 1000
    ss  = tmp % 100
    tmp = tmp / 100
    mm  = tmp % 100
    tmp = tmp / 100
    hh  = tmp
    return   (( hh * 60 + mm ) * 60 + ss ) * 1000 + ms

def daymillis2mstime(time):
    ms  = time % 1000
    tmp = time / 1000
    ss  = tmp % 60
    tmp = tmp / 60
    mm  = tmp % 60
    tmp = tmp / 60
    hh  = tmp % 60 
    return ((hh * 100 + mm ) * 100 + ss) * 1000 + ms
    
def time2daymillis(time):
    return mstime2daymillis(time*1000)

def get_next_date(date):
    date_str = "%d"%date
    t1 = dt.datetime.strptime(date_str, "%Y%m%d")
    t2 = t1 + dt.timedelta(days=1)
    return t2.date().year * 10000 + t2.date().month*100 + t2.date().day

def get_pre_date(date):
    date_str = "%d"%date
    t1 = dt.datetime.strptime(date_str, "%Y%m%d")
    t2 = t1 - dt.timedelta(days=1)
    return t2.date().year * 10000 + t2.date().month*100 + t2.date().day
    
def get_dt_time(bar):
    time = bar.get("time")
#     time = daymillis2mstime(int(bar.get("time"))) / 1000
    date = bar.get("date")
    year = date / 10000
    month = date / 100 % 100
    day   = date % 100
    hh    = time / 10000
    mm    = time / 100 % 100
    ss    = time % 100   
    return dt.datetime(year,month,day,hh,mm,ss,0)    


class TimeUtil:
    def __init__(self):
        self.night_begin = time2daymillis(200000)
        self.night_end   = time2daymillis(30000)
        self.oneday_millis = 24 * 60 * 60 * 1000 
        self.calendar = pd.DataFrame.from_csv('./calendar.csv',parse_dates=False).index.tolist()
        aucttime = {
                    'SZ': [(93000,113000), (130000,150000)],
                    'SH': [(93000,113000), (130000,150000)],
                    'DCE':[(210000,23000), (90000,101500), (103000,113000),(133000,150000)],
                    'SHF':[(210000,23000), (90000,101500), (103000,113000),(133000,150000)],
                    'CZC':[(210000,23000), (90000,101500), (103000,113000),(133000,150000)],
                    'CFE_IF':[(93000,113000), (130000,150000)],
                    'CFE_BF':[(91500,113000), (130000,151500)]                   
                    }
        self.aucttime = {}
        for k,v in aucttime.items():
            bartime = []
            for pair in v:
                bartime.append( (time2daymillis(pair[0]), time2daymillis(pair[1])) )
          
            self.aucttime[k] = bartime 
     
            
    def daymillis_cmp(self,t1, t2):
        if (t1 > self.night_begin) :
            if (t2 > self.night_begin) :
                if t1 > t2 :
                    return 1
                elif t1 < t2 :
                    return -1
                else :
                    return 0
            else:
                return -1
        else :
            if (t2 > self.night_begin) :
                return 1
            else:
                if t1 > t2 :
                    return 1
                elif t1 < t2 :
                    return -1
                else :
                    return 0   
                 
    def get_code_mkt(self,symbol):
        code = ''      
        tmp = symbol.split('.')
        mkt = tmp[1]
        code = tmp[0]
        
        if mkt == 'CFE':
            if code.startswith('T'):
                mkt += '_BF'
            else:
                mkt += '_IF'
        return (code,mkt)
    
    def get_begintime(self,symbol):
        code, mkt = self.get_code_mkt(symbol)
        tradetime = self.aucttime.get(mkt)
        begintime = tradetime[0]
        return begintime
    
    def is_tradetime(self,symbol, time): 
        code, mkt = self.get_code_mkt(symbol)
        daymillis =  time
        tradetime = self.aucttime.get(mkt)
        for pair in tradetime:
            if self.daymillis_cmp(daymillis,pair[0]) >= 0 and  self.daymillis_cmp(daymillis,pair[1]) <= 0:
                return True
        return False
    
    def daymillis_minus(self,t1,t2):
        if t1 >= t2:
            return t1-t2
        else:
            return t1-t2 + self.oneday_millis
        
    def daymillis_plus(self,t1,t2):
        return (t1+t2)%self.oneday_millis
           
class tk2bar(object):
    '''
    classdocs
    '''
    def __init__(self, user, passwd):
        self.user = user
        self.passwd = passwd
        self.tkreader = TkReader()     
        if( not self.tkreader.login(user, passwd)):
            print("login failed")
            return False
        self.start_time = "21:00:00"
        self.end_time = "15:30:00"             
        self.bar_map = {}
        self.trade_date = 0
        self.cycle = 0
        self.timeutil = TimeUtil()
        self.trade_time_map = {}    
        
        
        '''
        Constructor
        '''
    def format_df(self, dict):        
        
        df = pd.DataFrame.from_dict(dict)
        df.loc[:,'time'] = df.loc[:,'time'].astype("int64")
        df.loc[:,'time'] = df.loc[:,'time'].apply(daymillis2mstime)
        df.loc[:,'time'] /= 1000
        df.rename(columns={'turnover':'turnover_total'}, inplace=True) 
        df.rename(columns={'volume':'volume_total'}, inplace=True)
        df.rename(columns={'turnover_inc':'turnover'}, inplace=True) 
        df.rename(columns={'volume_inc':'volume'}, inplace=True)
        
        df.loc[:,'volume'] = df.loc[:,'volume_total'] - df.loc[:,'volume_total'].shift(1)
        df.loc[:,'turnover'] = df.loc[:,'turnover_total'] - df.loc[:,'turnover_total'].shift(1)
        df.loc[0,'volume'] = df.loc[0,'volume_total']
        df.loc[0,'turnover'] = df.loc[0,'turnover_total']
        
        df.loc[:,'open'      ] *= 10000 
        df.loc[:,'high'      ] *= 10000 
        df.loc[:,'low'       ] *= 10000 
        df.loc[:,'close'     ] *= 10000 
        df.loc[:,'settle'    ] *= 10000    

        df.loc[:,'askprice1' ] *= 10000
        df.loc[:,'askprice2' ] *= 10000
        df.loc[:,'askprice3' ] *= 10000
        df.loc[:,'askprice4' ] *= 10000
        df.loc[:,'askprice5' ] *= 10000

        df.loc[:,'bidprice1' ] *= 10000
        df.loc[:,'bidprice2' ] *= 10000
        df.loc[:,'bidprice3' ] *= 10000
        df.loc[:,'bidprice4' ] *= 10000
        df.loc[:,'bidprice5' ] *= 10000 
        
        for field in integer_fields:            
            df.loc[:,field] = df.loc[:,field].astype("int64")   
        time = df.apply(get_dt_time, axis=1) 
        df.index = time          

        return df
    
    def get_trade_time(self, symbol):
        code,mkt = self.timeutil.get_code_mkt(symbol)
        if not self.trade_time_map.has_key(mkt):
            id = 0
            bar_times = []
            id_map = {}
            trade_time = self.timeutil.aucttime.get(mkt)
            for pair in trade_time:
                time = self.timeutil.daymillis_plus(pair[0], self.cycle)
                while self.timeutil.daymillis_cmp(time, pair[1]) <= 0:
                    bar_times.append(time)
                    id_map[time] = id
                    id += 1
                    time = self.timeutil.daymillis_plus(time, self.cycle)
            self.trade_time_map[mkt] = {'id_map': id_map, 'bar_times': bar_times}
        return  self.trade_time_map[mkt]       

    
    def tick2bar(self, file_name, trade_dt, mkt, freq, out_dir): 
       
        if (not self.tkreader.open(file_name, "", self.start_time, self.end_time)):
            print("can't open file %s " %file_name)
            return False           
            
        self.trade_date = trade_dt
        if trade_dt not in self.timeutil.calendar:
            return False
        dt_id = self.timeutil.calendar.index(trade_dt)
        self.night_date = self.timeutil.calendar[dt_id-1]
        self.midnight_date = get_next_date(self.night_date)
        
        if freq == '5M':
            self.cycle = 5 * 60 * 1000
        elif freq == '15M':
            self.cycle = 15 * 60 * 1000
        else:
            self.cycle = 60 * 1000

        out_file = mkt + str(trade_dt) + '-' + freq + '.h5'  
        out_file = out_dir + out_file 
        i = 0  
        tk = self.tkreader.get_next()
        while tk is not None:
            self.on_tick(tk)
            i += 1
            if i % 50000 == 0:
                print('read %d ticks'%i)
            tk = self.tkreader.get_next()
        
        data = pd.HDFStore(out_file,'a')
        for k,v in self.bar_map.items():
            try:
#                 print k
                self.makeup_bars(v)                
                bars = v.get('bars')
                df = self.format_df(bars)
                data[k] = df
            except Exception, e:
                traceback.print_exc()
        data.close()       
        #return data
    
    def makeup_bars(self, bardata):
        bars = bardata.get('bars')
        ids = bardata.get('ids')
        ids_size= len(ids)
        first_id = ids[0]   
        last_id  = ids[-1]     
        preclose = bardata.get('preclose')
       
        if(first_id > 0):
            for i in range(0,first_id):
                bars['open'][i] = preclose
                bars['high'][i] = preclose
                bars['low'][i] = preclose
                bars['close'][i] = preclose
        
        if last_id < len(bars['time'])-1:
            ids.append(len(bars['time'])-1)
            
        if  ids_size > 2:
            for i in range(1,ids_size):
                head = ids[i-1]
                tail = ids[i]
                if (head+1) < tail:
                    close = bars['close'][head]
                    bars['open'][head+1:tail]  = close
                    bars['high'][head+1:tail]  = close
                    bars['low'][head+1:tail]   = close
                    bars['close'][head+1:tail] = close
                    bars['volume'  ][head+1:tail] = bars['volume'  ][head]
                    bars['turnover'][head+1:tail] = bars['turnover'][head]
                    bars['oi'      ][head+1:tail] = bars['oi'      ][head]
                    bars['settle'  ][head+1:tail] = bars['settle'  ][head]
                    bars['trade_date'][head+1:tail] = bars['trade_date'][head]
      
                
    def new_bardata(self,bar_time_list):
        last = None
        first = None
        bars  = {}
        size = len(bar_time_list)
        for field in bar_fields :
            if field == 'time' :
                bars[field] = np.array(bar_time_list)
            else:
                bars[field] = np.zeros(size)
        time_array = bars['time']
        date_array = bars['date']
        for i in range(0,len(time_array)):            
            if time_array[i] > self.timeutil.night_begin:
                date_array[i] = self.night_date
            elif time_array[i] < self.timeutil.night_end:
                date_array[i] = self.midnight_date
            else:
                date_array[i] = self.trade_date 
        bars['low'] += 999999999.0       
        return {'bars': bars,'preclose':0.0, 'ids':[]}
    
    def new_bar(self):
        bar = {}
        for field in bar_fields :
            bar[field] = None
        return bar
        
    def get_bartime(self, daymillis):
        bartime = (daymillis / self.cycle + 1) * self.cycle
        return bartime
    

    def on_tick(self, tk):
        time   = tk['time']
        symbol = tk['symbol']
#         print(time,symbol,tk['volume'],tk['turnover'])
        daymillis = mstime2daymillis(time) 
        barmillis = self.get_bartime(daymillis)
        trade_time = self.get_trade_time(symbol)
        id_map = trade_time['id_map'] 
        bar_time_list = trade_time['bar_times']
        
        if not id_map.has_key(barmillis) :
            return
         
        if not self.timeutil.is_tradetime(symbol, daymillis):
            return         

        if not self.bar_map.has_key(symbol):
            self.bar_map[symbol] = self.new_bardata(bar_time_list)
         
        id = id_map.get(barmillis)
            
        bardata = self.bar_map.get(symbol)
        bardata['preclose'] = tk['preclose']
        barlist = bardata.get('bars') 
           
        if len(bardata['ids']) == 0 :
            bardata['ids'].append(id)
        elif bardata['ids'][-1] < id:
            bardata['ids'].append(id)         
       
        last = tk['last'] 
        if (barlist['open'][id] < 0.000001):
            barlist['open'][id] = last 
        if  barlist['high'][id] < last:
            barlist['high'][id] = last
        if  barlist['low'][id] > last:
            barlist['low'][id] = last
        barlist['close'][id] = last
        for field in update_fields:
            if tk.has_key(field):
                barlist[field][id] = tk[field]        

                
if __name__ == '__main__':   
    user = "phone number"
    passwd = "token number"           
    convert = tk2bar(user,passwd)
    try :
        convert.tick2bar('./SHF20171218.tk', 20171218, 'SHF','1M','./')
    except Exception, e:
        traceback.print_exc() 
        raise e
    
 
       
            
          
          
          
          