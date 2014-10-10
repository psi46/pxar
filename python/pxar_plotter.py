import ROOT
import numpy
import array

class Plotter(object):

    @staticmethod
    def create_th1(data, step, minimum, maximum, name, x_title, y_title):
        th1 = ROOT.TH1F(name, name, int(maximum-minimum), minimum, maximum)
        th1.SetDirectory(0)
        th1.GetXaxis().SetTitle(x_title)
        th1.GetYaxis().SetTitle(y_title)
        th1.SetDrawOption('HIST')
        th1.SetLineWidth(2)
        for idac, dac in enumerate(data):
            if(dac):
                th1.Fill(idac,dac[0].value)
        return th1
        
    @staticmethod
    def create_tgraph(data, name, x_title, y_title, minimum = None, maximum = None):
        xdata = list(xrange(len(data)))
        x = array.array('d', xdata)
        y = array.array('d', data)
        gr = ROOT.TGraph(len(x),x,y)
        #gr.SetDirectory(0)
        gr.SetTitle(name)
        gr.SetLineColor(4)
        gr.SetMarkerColor(4)
        gr.SetMarkerSize(0.5)
        gr.SetMarkerStyle(21)
        gr.GetXaxis().SetTitle(x_title)
        gr.GetYaxis().SetTitle(y_title)
        gr.SetDrawOption('Acp')
        gr.SetLineWidth(2)
        return gr

    @staticmethod
    def create_th2(data, len_x, len_y, name, x_title, y_title):
        th2 = ROOT.TH2F(name, name, len_x, 0, len_x , len_y, 0, len_y)
        th2.SetDirectory(0)
        th2.GetXaxis().SetTitle(x_title)
        th2.GetYaxis().SetTitle(y_title)
        th2.SetDrawOption('COLZ')
        for px in data:
            th2.SetBinContent(px.column + 1, px.row + 1, px.value)
        return th2
    
    def matrix_to_th2(self, matrix, name, x_title, y_title):
        dim = matrix.shape
        return self.create_th2(matrix, dim[0], dim[1], name, x_title, y_title)
