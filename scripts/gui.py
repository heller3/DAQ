from tkinter import *
from conf import *

# Very important : nothing LOGICAL actually done here !
# Only display and buttons

class Row:
    def __init__(self, main_window, component, label, row_index): #ValueDAC,ValueVref,ValueDac_binaire,ValueDac_dec,max_speed_hz):
        self.main_window = main_window

        # input
        self.component = component # ref to the component object
        self.enabled = False
        self.text=label
        self.label = Label(main_window.master, text=label)
        self.row_index = row_index
        self.entry = Entry(main_window.master, width=15, state='disabled')
        self.enable_button_text = StringVar()
        self.enable_button = Button(main_window.master, textvariable=self.enable_button_text, width=10, command=lambda: self.toggle_enable())
        self.send_button = Button(main_window.master, text=SEND_TEXT, width=10, state='disabled', command=lambda: self.send_entry())
        # display
        self.blanc = Label(main_window.master, text=" ",width=5) # aucun intérêt de le déclarer en attribut
        self.indicator_text = Label(main_window.master,text=label)
        self.indicator_value = Label(main_window.master,text='')  # blank value on start

        # self.TimerInterval = 500


    def show_row(self):
        # input
        self.label.grid(row=self.row_index,column=0)
        self.entry.grid(row=self.row_index,column=1)
        self.enable_button_text.set(ENABLE_TEXT)
        self.enable_button.grid(row=self.row_index,column=2)
        self.send_button.grid(row=self.row_index,column=3)
        # display
        self.blanc.grid(row=self.row_index,column=4)
        self.indicator_text.grid(row=self.row_index,column=5)
        self.indicator_value.grid(row=self.row_index,column=6)

        '''
        Callbacks
        '''
    def toggle_enable(self):
        # normal and disabled fonction for the boutton
        if self.enabled :
            self.entry.config(state='disabled')
            self.send_button.config(state='disabled')
            self.enable_button_text.set(ENABLE_TEXT)
            self.enabled = False

        else:
            self.entry.config(state='normal')
            self.send_button.config(state='normal')
            self.enable_button_text.set(DISABLE_TEXT)
            self.enabled = True

    def send_entry(self):
        # the index in row_labels = the row index - 1 (because the first one is only text)
##        output = self.component.send_value(self.row_index - 1, self.entry.get())
        output = self.component.send_value(self.row_index - 1, self.entry.get())

        # refresh the display with the value that has been sent
        if(output): # if output == None we shouldn't refresh
            self.update_value(output)

        self.main_window.update_values()

    # def continuous_hv_display(self):
    #
    #     value = self.component.read_voltage()
    #     self.update_value(value)
    #     self.after(self.TimerInterval,self.continuous_hv_display)

    def update_value(self, value):
        self.indicator_value.configure(text=value)




class MainWindow:

    def __init__(self, component):
        self.master = Tk()
        self.component = component
        self.rows = []
        self.TimerInterval = 500
        # self.continuous_display()

    def make_layout(self, labels):

        self.master.title(TITLE)

        i=0
        # Titles
        Label(self.master,text=VOLTAGE_TITLE).grid(row=i,column=1)
        # Rows

        for label in labels :
            i+=1
            #TODO: initialize the valu
            row = Row(self, self.component, label, i)
            self.rows.append(row)
            row.show_row()

        self.update_values() # update the values on the right

        self.hvon = Button(self.master, text='HV ON', command=lambda: self.switch_on_hv())
        self.hvon.grid(row=12,column=1)

        self.hvoff = Button(self.master, text='HV OFF', command=lambda: self.switch_off_hv())
        self.hvoff.grid(row=12,column=2)

        self.reset = Button(self.master, text='reset', command=lambda: self.reset_values())
        self.reset.grid(row=12,column=3)

        self.send_all = Button(self.master, text="send_all", command=lambda: self.send_enabled_values())
        self.send_all.grid(row=12,column=4)

        # self.display = Button(self.master, text='Activate Display', command=lambda: self.continuous_display())
        # self.display.grid(row=13,column=1)

        self.v_monitor_label = Label(self.master,text="Voltage")
        self.v_monitor_label.grid(row=14,column=1)

        self.v_monitor = Label(self.master,text='---')  # blank value on start
        self.v_monitor.grid(row=14,column=2)

        self.i_monitor_label = Label(self.master,text="Current")
        self.i_monitor_label.grid(row=15,column=1)

        self.i_monitor = Label(self.master,text='---')  # blank value on start
        self.i_monitor.grid(row=15,column=2)
        
        self.init_comm()
        self.set_low_v()
        self.continuous_display()

        self.master.mainloop()

    def update_values(self):
        values = self.component.Voltage
        # zip is a function that allows to iterate both lists a the same time
        for row, value in zip(self.rows, values):
            row.update_value(value)

    def reset_values(self):
        self.component.reset()
        self.update_values()

    def send_enabled_values(self):
        # Little trick : we start at the end to be sure values of vref are updated
        for row in reversed(self.rows):
            if row.enabled:
                row.send_entry()

    def switch_on_hv(self):
        self.component.switch_on_hv()

    def switch_off_hv(self):
        self.component.switch_off_hv()
        self.component.send_value(0,"10")
    
    def init_comm(self):
        self.component.begin_com()
        
        
    def set_low_v(self):
        self.component.send_value(0,"10")

    def continuous_display(self):

        v_value = self.component.read_voltage()
        i_value = self.component.read_current()

        self.update_v_display(v_value)
        self.update_i_display(i_value)

        self.v_monitor.after(self.TimerInterval,self.continuous_display)

    def update_v_display(self, value):
        self.v_monitor.configure(text=value)

    def update_i_display(self, value):
        self.i_monitor.configure(text=value)
