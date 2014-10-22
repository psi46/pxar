import ROOT
import numpy
import array

class Plotter(object):

    @staticmethod
    def create_th1(data, minimum, maximum, name, x_title, y_title):
        th1 = ROOT.TH1F(name, name, len(data), minimum, maximum)
        th1.SetDirectory(0)
        th1.GetXaxis().SetTitle(x_title)
        th1.GetYaxis().SetTitle(y_title)
        th1.SetDrawOption('HIST')
        th1.SetLineWidth(2)
        for ix, x in enumerate(data):
            th1.Fill(ix,x)
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
    def create_th2(data, x_min, x_max, y_min, y_max, name, x_title, y_title, z_title):
        th2 = ROOT.TH2F(name, name, data.shape[0], x_min, x_max, data.shape[1], y_min, y_max)
        th2.SetDirectory(0)
        th2.GetXaxis().SetTitle(x_title)
        th2.GetYaxis().SetTitle(y_title)
        th2.GetZaxis().SetTitle(z_title)
        th2.SetDrawOption('COLZ')
        for ix, x in enumerate(data):
            for iy, y in enumerate(x):
                th2.SetBinContent(ix, iy, y)
        return th2
    
    def matrix_to_th2(self, matrix, name, x_title, y_title):
        dim = matrix.shape
        return self.create_th2(matrix, dim[0], dim[1], name, x_title, y_title)
