import jzs
import random
from datetime import datetime
import pandas as pd
import traceback

class QmsSystem :

    def __init__(self) :
        self._qms = None
        self._strategy_id = None

    def connect(self, qms_addr, strategy_id):
        """
            qms_addr    tcp://127.0.0.1:9000
            strategy_id integer 1
        """
        self._strategy_id = strategy_id

        print "Use QMS:", qms_addr

        self._qms = jzs.QmsClient()
        self._qms.connect(qms_addr)
    
    def bar1m(self, code, want_inc = False):
        if not self._qms : return None, "qms is not inited"
    
        qms = self._qms
        try :
            bar = qms.getBar1M(code)
            #starttime=datetime.now()
            result, last = _bar1m_to_dataframe(bar, want_inc)

            result = pd.DataFrame(result)
            result = result.transpose()

            return result, last
        except Exception, e:
            traceback.print_exc()
            return None, str(e)

    
    def quote(self, code):
        if not self._qms: return None
    
        try :
            return self._qms.getMarketQuote(code)

        except:
            traceback.print_exc()
            return None
        
def _string_to_date(date,time):
    time=(time)/1000
    hour=int(time/3600)
    minute=int((time-3600*hour)/60)
    second=time-3600*hour-60*minute
    date=str(date)
    #t = datetime.strptime(date+str(hour)+str(minute)+str(second), "%Y-%m-%d %H:%M:%S")
    t=datetime(int(date[0:4]),int(date[4:6]),int(date[6:8]),hour,minute,second)
    return t

def _bar1m_to_dataframe(bar_file, want_inc):
    bar_len=len(bar_file.bar)

    key_list  = ['open','high','low','close', 'match_item', 'volume',     'turnover',     'interest', 'vwap']
    name_list = ['open','high','low','close', 'match_item', 'volume',     'turnover',     'interest', 'vwap']

    if want_inc:
        key_list=['open','high','low','close', 'match_item', 'volume_inc', 'turnover_inc', 'interest_inc', 'volume',     'turnover',     'interest', 'vwap']
        name_list = ['open','high','low','close', 'match_item', 'volume',     'turnover',   'interest',  'total_volume', 'total_turnover',  'total_interest', 'vwap']

    result = {}
    for i in xrange(0,bar_len):
        bar_temp = {}
        time_temp =_string_to_date(bar_file.bar[i].date, bar_file.bar[i].time)
        for k in xrange(len(key_list)):
            bar_temp[name_list[k]] = bar_file.bar[i].__getattribute__(key_list[k])

        result[time_temp] = bar_temp


    key_last_list=['jzcode','time','open','high','low','last','volume','turnover','interest','close','settle','delta',\
            'iopv','avgbidpx','totbidvol','avgaskpx','totaskvol', 'quoteage']
    key_ab_list=['bidPrice','bidVolume','askPrice','askVolume']
    key_qs_list=['date','tradeday','uplimit','downlimit','preinterest','preclose','presettle','predelta']
    last={}
    for key_last_temp in key_last_list:
        last[key_last_temp]=bar_file.last.__getattribute__(key_last_temp)
    last['ab']={}
    for key_ab_temp in key_ab_list:
        last['ab'][key_ab_temp]=bar_file.last.ab.__getattribute__(key_ab_temp)
    last['qs']={}
    for key_qs_temp in key_qs_list:
        last['qs'][key_qs_temp]=bar_file.last.qs.__getattribute__(key_qs_temp)
    
    return result,last

