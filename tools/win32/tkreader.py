import _tkreader
import traceback
import pandas as pd
import datetime as dt

class TkReader:
    def __init__(self):
        self._handle = 0
        
    def __del__(self):
        self.close()

    def login(self, username, passwd):
        self._handle = _tkreader.login(username, passwd)
        if(self._handle == 0):
            print("login failed")
            return False
        else:
            return True

    def open(self, file_name, universe, start_time, end_time):
        if(self._handle == 0) :
            print("please login first")
        if(_tkreader.open(self._handle, file_name,universe,start_time,end_time) == 0):
            print("open file failed")
            return False
        else:
            return True

    def get_next(self):
        return _tkreader.get_next_tick(self._handle)

    def close(self):
        if self._handle:
            _tkreader.close(self._handle)
            self._handle = 0

def to_datetime(date,time):
    y = date / 10000
    m = date / 100 % 100
    d = date % 100
    Z = time % 1000 * 1000
    tmp = time / 1000
    S = tmp % 100
    M = tmp / 100 % 100
    H = tmp / 10000
    return dt.datetime(y,m,d,H,M,S,Z) 

def tk_to_h5(user, passwd, tkfile, begin_time, end_time, h5file):    
    try:
        file = tkfile
        reader = TkReader()
        if( not reader.login(user, passwd)):
            return False
        if( not reader.open(file, "", begin_time, end_time)):
            return False
        print("open finished")
        tk = reader.get_next()

        fname = h5file
        store = pd.HDFStore(fname, "a")
        df = pd.DataFrame()

        data = {}

        line = 0
        while tk != None:
            
            symbol = tk['symbol']
            
            if not data.has_key(symbol):
                inst_data = {}
                inst_data['last'      ] = []
                inst_data['vwap'      ] = []
                inst_data['open'      ] = []
                inst_data['high'      ] = []
                inst_data['low'       ] = []
                inst_data['close'     ] = []
                inst_data['settle'    ] = []
                inst_data['iopv'      ] = []
                inst_data['oi'        ] = []
                inst_data['volume'    ] = []
                inst_data['turnover'  ] = []             
                inst_data['date'      ] = []
                inst_data['trade_date'] = []
                inst_data['time'      ] = []
                inst_data['limit_up'  ] = []
                inst_data['limit_down'] = []
                inst_data['preclose'  ] = []
                inst_data['presettle' ] = []
                inst_data['preoi'     ] = []
                inst_data['index'     ] = []

                if(tk.has_key("askprice1")):
                    inst_data['askprice1'] = []
                if(tk.has_key("askprice2")):
                    inst_data['askprice2'] = []
                if(tk.has_key("askprice3")):
                    inst_data['askprice3'] = []
                if(tk.has_key("askprice4")):
                    inst_data['askprice4'] = []
                if(tk.has_key("askprice5")):
                    inst_data['askprice5'] = []

                if(tk.has_key("bidprice1")):
                    inst_data['bidprice1'] = []
                if(tk.has_key("bidprice2")):
                    inst_data['bidprice2'] = []
                if(tk.has_key("bidprice3")):
                    inst_data['bidprice3'] = []
                if(tk.has_key("bidprice4")):
                    inst_data['bidprice4'] = []
                if(tk.has_key("bidprice5")):
                    inst_data['bidprice5'] = []
    
                if(tk.has_key("askvolume1")):
                    inst_data['askvolume1'] = []
                if(tk.has_key("askvolume2")):
                    inst_data['askvolume2'] = []
                if(tk.has_key("askvolume3")):
                    inst_data['askvolume3'] = []
                if(tk.has_key("askvolume4")):
                    inst_data['askvolume4'] = []
                if(tk.has_key("askvolume5")):
                    inst_data['askvolume5'] = []
    
                if(tk.has_key("bidvolume1")):
                    inst_data['bidvolume1'] = []
                if(tk.has_key("bidvolume2")):
                    inst_data['bidvolume2'] = []
                if(tk.has_key("bidvolume3")):
                    inst_data['bidvolume3'] = []
                if(tk.has_key("bidvolume4")):
                    inst_data['bidvolume4'] = []
                if(tk.has_key("bidvolume5")):
                    inst_data['bidvolume5'] = []

                data[symbol] = inst_data


            inst_data = data[symbol]       
            inst_data['last'      ].append(tk['last'      ]) 
            inst_data['vwap'      ].append(tk['vwap'      ]) 
            inst_data['open'      ].append(tk['open'      ]) 
            inst_data['high'      ].append(tk['high'      ]) 
            inst_data['low'       ].append(tk['low'       ]) 
            inst_data['close'     ].append(tk['close'     ]) 
            inst_data['settle'    ].append(tk['settle'    ]) 
            inst_data['iopv'      ].append(tk['iopv'      ]) 
            inst_data['oi'        ].append(tk['oi'        ]) 
            inst_data['volume'    ].append(tk['volume'    ]) 
            inst_data['turnover'  ].append(tk['turnover'  ])                 
            inst_data['limit_up'  ].append(tk['limit_up'  ])
            inst_data['limit_down'].append(tk['limit_down'])
            inst_data['preclose'  ].append(tk['preclose'  ])
            inst_data['presettle' ].append(tk['presettle' ])
            inst_data['preoi'     ].append(tk['preoi'     ])
          
            inst_data['date'      ].append(tk['date'      ]) 
            inst_data['trade_date'].append(tk['trade_date']) 
            inst_data['time'      ].append(tk['time'      ]) 
            inst_data['index'     ].append(to_datetime(tk['date'], tk['time']))

            if(tk.has_key("askprice1")):
                inst_data['askprice1'].append(tk['askprice1'])
            else: 
                inst_data['askprice1'].append(0.0)

            if(tk.has_key("askprice2")):
                inst_data['askprice2'].append(tk['askprice2'])
            else: 
                inst_data['askprice2'].append(0.0)

            if(tk.has_key("askprice3")):
                inst_data['askprice3'].append(tk['askprice3'])
            else: 
                inst_data['askprice3'].append(0.0)

            if(tk.has_key("askprice4")):
                inst_data['askprice4'].append(tk['askprice4'])
            else: 
                inst_data['askprice4'].append(0.0)

            if(tk.has_key("askprice5")):
                inst_data['askprice5'].append(tk['askprice5'])
            else: 
                inst_data['askprice5'].append(0.0)

            if(tk.has_key("bidprice1")):
                inst_data['bidprice1'].append(tk['bidprice1'])
            else: 
                inst_data['bidprice1'].append(0.0)

            if(tk.has_key("bidprice2")):
                inst_data['bidprice2'].append(tk['bidprice2'])
            else: 
                inst_data['bidprice2'].append(0.0)

            if(tk.has_key("bidprice3")):
                inst_data['bidprice3'].append(tk['bidprice3'])
            else: 
                inst_data['bidprice3'].append(0.0)

            if(tk.has_key("bidprice4")):
                inst_data['bidprice4'].append(tk['bidprice4'])
            else: 
                inst_data['bidprice4'].append(0.0)

            if(tk.has_key("bidprice5")):
                inst_data['bidprice5'].append(tk['bidprice5'])
            else: 
                inst_data['bidprice5'].append(0.0)

            if(tk.has_key("askvolume1")):
                inst_data['askvolume1'].append(tk['askvolume1'])
            else: 
                inst_data['askvolume1'].append(0.0)

            if(tk.has_key("askvolume2")):
                inst_data['askvolume2'].append(tk['askvolume2'])
            else: 
                inst_data['askvolume2'].append(0.0)

            if(tk.has_key("askvolume3")):
                inst_data['askvolume3'].append(tk['askvolume3'])
            else: 
                inst_data['askvolume3'].append(0.0)

            if(tk.has_key("askvolume4")):
                inst_data['askvolume4'].append(tk['askvolume4'])
            else: 
                inst_data['askvolume4'].append(0.0)

            if(tk.has_key("askvolume5")):
                inst_data['askvolume5'].append(tk['askvolume5'])
            else: 
                inst_data['askvolume5'].append(0.0)

            if(tk.has_key("bidvolume1")):
                inst_data['bidvolume1'].append(tk['bidvolume1'])
            else: 
                inst_data['bidvolume1'].append(0.0)

            if(tk.has_key("bidvolume2")):
                inst_data['bidvolume2'].append(tk['bidvolume2'])
            else: 
                inst_data['bidvolume2'].append(0.0)

            if(tk.has_key("bidvolume3")):
                inst_data['bidvolume3'].append(tk['bidvolume3'])
            else: 
                inst_data['bidvolume3'].append(0.0)

            if(tk.has_key("bidvolume4")):
                inst_data['bidvolume4'].append(tk['bidvolume4'])
            else: 
                inst_data['bidvolume4'].append(0.0)

            if(tk.has_key("bidvolume5")):
                inst_data['bidvolume5'].append(tk['bidvolume5'])
            else: 
                inst_data['bidvolume5'].append(0.0)


            tk = reader.get_next()
            line = line + 1
            if (line % 10000 == 0):
                print("write " + str(line) + " ticks")
        
        for (symbol, value) in data.items():
            print('symbol=', symbol)
            df = pd.DataFrame.from_dict(value)
            df.index = df['index']
            df.drop(['index'], axis = 1, inplace=True)            

            df.loc[:,'last'      ] *= 10000 
            df.loc[:,'vwap'      ] *= 10000 
            df.loc[:,'open'      ] *= 10000 
            df.loc[:,'high'      ] *= 10000 
            df.loc[:,'low'       ] *= 10000 
            df.loc[:,'close'     ] *= 10000 
            df.loc[:,'settle'    ] *= 10000 
            df.loc[:,'iopv'      ] *= 10000                  
            df.loc[:,'limit_up'  ] *= 10000 
            df.loc[:,'limit_down'] *= 10000 
            df.loc[:,'preclose'  ] *= 10000 
            df.loc[:,'presettle' ] *= 10000

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


            df.loc[:,'last'      ] = df.loc[:,'last'      ].astype("int64")
            df.loc[:,'vwap'      ] = df.loc[:,'vwap'      ].astype("int64")
            df.loc[:,'open'      ] = df.loc[:,'open'      ].astype("int64")
            df.loc[:,'high'      ] = df.loc[:,'high'      ].astype("int64")
            df.loc[:,'low'       ] = df.loc[:,'low'       ].astype("int64")
            df.loc[:,'close'     ] = df.loc[:,'close'     ].astype("int64")
            df.loc[:,'settle'    ] = df.loc[:,'settle'    ].astype("int64")
            df.loc[:,'iopv'      ] = df.loc[:,'iopv'      ].astype("int64")
            df.loc[:,'oi'        ] = df.loc[:,'oi'        ].astype("int64")
            df.loc[:,'volume'    ] = df.loc[:,'volume'    ].astype("int64")
            df.loc[:,'turnover'  ] = df.loc[:,'turnover'  ].astype("int64")
            df.loc[:,'limit_up'  ] = df.loc[:,'limit_up'  ].astype("int64")
            df.loc[:,'limit_down'] = df.loc[:,'limit_down'].astype("int64")
            df.loc[:,'preclose'  ] = df.loc[:,'preclose'  ].astype("int64")
            df.loc[:,'presettle' ] = df.loc[:,'presettle' ].astype("int64")
            df.loc[:,'preoi'     ] = df.loc[:,'preoi'     ].astype("int64")

            df.loc[:,'askprice1' ] = df.loc[:,'askprice1' ].astype("int64")
            df.loc[:,'askprice2' ] = df.loc[:,'askprice2' ].astype("int64")
            df.loc[:,'askprice3' ] = df.loc[:,'askprice3' ].astype("int64")
            df.loc[:,'askprice4' ] = df.loc[:,'askprice4' ].astype("int64")
            df.loc[:,'askprice5' ] = df.loc[:,'askprice5' ].astype("int64")

            df.loc[:,'bidprice1' ] = df.loc[:,'bidprice1' ].astype("int64")
            df.loc[:,'bidprice2' ] = df.loc[:,'bidprice2' ].astype("int64")
            df.loc[:,'bidprice3' ] = df.loc[:,'bidprice3' ].astype("int64")
            df.loc[:,'bidprice4' ] = df.loc[:,'bidprice4' ].astype("int64")
            df.loc[:,'bidprice5' ] = df.loc[:,'bidprice5' ].astype("int64")

            df.loc[:,'askvolume1' ] = df.loc[:,'askvolume1' ].astype("int64")
            df.loc[:,'askvolume2' ] = df.loc[:,'askvolume2' ].astype("int64")
            df.loc[:,'askvolume3' ] = df.loc[:,'askvolume3' ].astype("int64")
            df.loc[:,'askvolume4' ] = df.loc[:,'askvolume4' ].astype("int64")
            df.loc[:,'askvolume5' ] = df.loc[:,'askvolume5' ].astype("int64")

            df.loc[:,'bidvolume1' ] = df.loc[:,'bidvolume1' ].astype("int64")
            df.loc[:,'bidvolume2' ] = df.loc[:,'bidvolume2' ].astype("int64")
            df.loc[:,'bidvolume3' ] = df.loc[:,'bidvolume3' ].astype("int64")
            df.loc[:,'bidvolume4' ] = df.loc[:,'bidvolume4' ].astype("int64")
            df.loc[:,'bidvolume5' ] = df.loc[:,'bidvolume5' ].astype("int64")

            store[symbol] = df

        store.close()

    except Exception as e:
        traceback.print_exc() 

def read_h5(filename,symbol):   
    data = pd.read_hdf(filename,symbol)
    print(data)
        
if __name__ == '__main__':
    user = "phone number"
    passwd = "token number"
    tkfile = "DCE20171214.tk"
    h5file = "DCE20171214.h5"
    start_time = "21:00:00"
    end_time = "15:00:00"

    tk_to_h5(user, passwd,tkfile,start_time,end_time,h5file)
    symbol = "i1801.DCE"
    read_h5(h5file,symbol)
        
    pass