// RPC makeshift functions

#include "rpc_impl.h"
#include "constants.h"
#include "log.h"

#define DAQ_READ_SIZE 32768

int8_t CTestboard::fallback_Daq_Enable(int32_t block) {

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";
  Daq_Open(block, 0);
  if(TBM_Present()) {
    Daq_Select_Deser400();
    Daq_Open(block, 1);
  }
  //FIXME deseradjust
  else { Daq_Select_Deser160(4); }
  Daq_Start(0);
  if(TBM_Present()) { Daq_Start(1); }
  return 1;
}

int8_t CTestboard::fallback_Daq_Disable() {

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";
  Daq_Stop(0);
  if(TBM_Present()) { Daq_Stop(1); }

  Daq_Close(0);
  if(TBM_Present()) { Daq_Close(1); }

  return 1;
}

int8_t CTestboard::fallback_Daq_Read(vector<uint16_t> &data, uint16_t daq_read_size, uint32_t &n) {

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  vector<uint16_t> data1;
  Daq_Read(data, daq_read_size, n, 0);

  if(TBM_Present()) { 
    Daq_Read(data1, daq_read_size, n, 1);
    data.insert( data.end(), data1.begin(), data1.end() );
  }
  return 1;
}

int8_t CTestboard::fallback_TrimChip(vector<int16_t> &trim) {
  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  for (int col = 0; col < ROC_NUMCOLS; col++) {
    for (int row = 0; row < ROC_NUMROWS; row++) {
      roc_Pix_Trim(col, row, trim[col * ROC_NUMROWS + row]);
    }
  }
  trim.clear();
  return 1;
}

int16_t CTestboard::fallback_CalibrateMap(int16_t nTriggers, vectorR<int16_t> &nReadouts, vectorR<int32_t> &PHsum, vectorR<uint32_t> &adress)
{
  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  int16_t ok = -1;
  uint32_t avail_size = 0;

  nReadouts.clear();
  PHsum.clear();
  adress.clear();

  nReadouts.resize(ROC_NUMCOLS * ROC_NUMROWS, 0);
  PHsum.resize(ROC_NUMCOLS * ROC_NUMROWS, 0);
  adress.resize(ROC_NUMCOLS * ROC_NUMROWS, 0);

  fallback_Daq_Enable(DAQ_READ_SIZE);
  vector<uint16_t> data;
  vector<uint16_t> data2;

  vector<uint16_t> n;
  vector<uint16_t> ph;
  vector<uint32_t> adr;



  for (uint8_t col = 0; col < ROC_NUMCOLS; col++) {
    roc_Col_Enable(col, true);
    for (uint8_t row = 0; row < ROC_NUMROWS; row++) {
      //arm
      roc_Pix_Cal(col, row, false);
      uDelay(5);
      for (uint8_t trigger = 0; trigger < nTriggers; trigger++) {
	//send triggers
	Pg_Single();
	uDelay(4);
      }
      // clear
      roc_ClrCal();
    }

    //read data
    data.clear();
    data2.clear();
    Daq_Read(data, DAQ_READ_SIZE, avail_size, 0);
    if (TBM_Present()){
      avail_size=0;
      Daq_Read(data2, DAQ_READ_SIZE, avail_size, 1);
    }

    //decode readouts
    n.clear();
    ph.clear();
    adr.clear();
    ok = fallback_Decode(data, n, ph, adr);
    ok = fallback_Decode(data2, n, ph, adr);

    int colR = -1, rowR = -1;

    for (unsigned int i = 0; i<adr.size();i++){
      rowR = adr[i] & 0xff;
      colR = (adr[i] >> 8) & 0xff;
      if (0 <= colR && colR < ROC_NUMCOLS && 0 <= rowR && rowR < ROC_NUMROWS){
	nReadouts[colR * ROC_NUMROWS + rowR] += n[i];
	PHsum[colR * ROC_NUMROWS + rowR] += ph[i];
	adress[colR * ROC_NUMROWS + rowR] = adr[i];
      }
    }

    roc_Col_Enable(col, false);
  }

  fallback_Daq_Disable();
  return ok;
}

int8_t CTestboard::fallback_CalibrateReadouts(int16_t nTriggers, int16_t &nReadouts, int32_t &PHsum){

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  nReadouts = 0;
  PHsum = 0;
  uint32_t avail_size = 0;
  int16_t ok = -1;

  vector<uint16_t> nhits, ph;
  vector<uint32_t> adr;
  vector<uint16_t> data;
  uDelay(5);

  for (int16_t i = 0; i < nTriggers; i++)
    {
      Pg_Single();
      uDelay(4);
    }

  fallback_Daq_Read(data, DAQ_READ_SIZE, avail_size);

  ok = fallback_Decode(data, nhits, ph, adr);

  for (size_t i = 0; i < adr.size(); i++) {
    nReadouts+= nhits[i];
    PHsum+= ph[i];;
  }

  return ok;
}


int8_t CTestboard::fallback_CalibrateDacDacScan(int16_t nTriggers, int16_t col, int16_t row, int16_t dacReg1,
					   int16_t dacLower1, int16_t dacUpper1, int16_t dacReg2, int16_t dacLower2, int16_t dacUpper2, vector<int16_t> &nReadouts, vector<int32_t> &PHsum) {

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called."; 
  int16_t n;
  int32_t ph;

  roc_Col_Enable(col, true);
  roc_Pix_Cal(col, row, false);
  uDelay(5);
  fallback_Daq_Enable(DAQ_READ_SIZE);
  for (int i = dacLower1; i < dacUpper1; i++)
    {
      roc_SetDAC(dacReg1, i);
      for (int k = dacLower2; k < dacUpper2; k++)
	{
	  roc_SetDAC(dacReg2, k);
	  fallback_CalibrateReadouts(nTriggers, n, ph);
	  nReadouts.push_back(n);
	  PHsum.push_back(ph);
	}
    }
  fallback_Daq_Disable();
  roc_ClrCal();
  roc_Col_Enable(col, false);

  return 1;
}

void CTestboard::fallback_DecodeTbmHeader(unsigned int raw, int16_t &evNr, int16_t &stkCnt)
{
  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";
  evNr = raw >> 8;
  stkCnt = raw & 6;
}

void CTestboard::fallback_DecodeTbmTrailer(unsigned int raw, int16_t &dataId, int16_t &data)
{
  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";
  dataId = (raw >> 6) & 0x3;
  data   = raw & 0x3f;
}

void CTestboard::fallback_DecodePixel(unsigned int raw, int16_t &n, int16_t &ph, int16_t &col, int16_t &row) 
{
  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";
  
  n = 1;
  ph = (raw & 0x0f) + ((raw >> 1) & 0xf0);
  raw >>= 9;
  int c =    (raw >> 12) & 7;
  c = c*6 + ((raw >>  9) & 7);
  int r =    (raw >>  6) & 7;
  r = r*6 + ((raw >>  3) & 7);
  r = r*6 + ( raw        & 7);
  row = 80 - r/2;
  col = 2*c + (r&1);
}

int8_t CTestboard::fallback_Decode(const std::vector<uint16_t> &data, std::vector<uint16_t> &n, std::vector<uint16_t> &ph, std::vector<uint32_t> &adr)
{ 

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  //  uint32_t words_remaining = 0;
  uint16_t hdr, trl;
  unsigned int raw;
  int16_t n_pix = 0, ph_pix = 0, col = 0, row = 0, evNr = 0, stkCnt = 0, dataId = 0, dataNr = 0;
  int16_t roc_n = -1;
  int16_t tbm_n = 1;
  uint32_t address;
  int pos = 0;

  //Module readout
  if (TBM_Present()){
    LOG(pxar::logDEBUGRPC) << "TBM detected, decoding DESER400 data.";
    for (size_t i = 0; i < data.size(); i++) {
      int d = data[i] & 0xf;
      int q = (data[i]>>4) & 0xf;
      switch (q)
	{
	case  0: break;

	case  1: raw = d; break;
	case  2: raw = (raw<<4) + d; break;
	case  3: raw = (raw<<4) + d; break;
	case  4: raw = (raw<<4) + d; break;
	case  5: raw = (raw<<4) + d; break;
	case  6: raw = (raw<<4) + d;
	  fallback_DecodePixel(raw, n_pix, ph_pix, col, row);
	  n.push_back(n_pix);
	  ph.push_back(ph_pix);
	  address = tbm_n;
	  address = (address << 8) + roc_n;
	  address = (address << 8) + col;
	  address = (address << 8) + row;
	  adr.push_back(address);
	  break;

	case  7: roc_n++; break;

	case  8: hdr = d; break;
	case  9: hdr = (hdr<<4) + d; break;
	case 10: hdr = (hdr<<4) + d; break;
	case 11: hdr = (hdr<<4) + d; 
	  fallback_DecodeTbmHeader(hdr, evNr, stkCnt);
	  tbm_n = tbm_n ^ 0x01;
	  roc_n = -1;
	  break;

	case 12: trl = d; break;
	case 13: trl = (trl<<4) + d; break;
	case 14: trl = (trl<<4) + d; break;
	case 15: trl = (trl<<4) + d;
	  fallback_DecodeTbmTrailer(trl, dataId, dataNr);
	  break;
	}
    }
  }
  //Single ROC
  else {
    LOG(pxar::logDEBUGRPC) << "Decoding DESER160 data.";
    while (!(pos >= int(data.size()))) {
      // check header
      if ((data[pos] & 0x8ffc) != 0x87f8)
	return -2; // wrong header
      int hdr = data[pos++] & 0xfff;
      // read pixels while not data end or trailer
      while (!(pos >= int(data.size()) || (data[pos] & 0x8000))) {
	// store 24 bits in raw
	raw = (data[pos++] & 0xfff) << 12;
	if (pos >= int(data.size()) || (data[pos] & 0x8000))
	  return -3; // incomplete data
	raw += data[pos++] & 0xfff;
	fallback_DecodePixel(raw, n_pix, ph_pix, col, row);
	n.push_back(n_pix);
	ph.push_back(ph_pix);
	address = 0;
	address = (address << 8) ;
	address = (address << 8) + col;
	address = (address << 8) + row;
	adr.push_back(address);
      }
    }
  }
  return 1;
}


int32_t CTestboard::fallback_Threshold(int32_t start, int32_t step, int32_t thrLevel,
			      int32_t nTrig, int32_t dacReg) {

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  int32_t threshold = start, newValue, oldValue, result;
  int stepAbs;
  if (step < 0)
    stepAbs = -step;
  else
    stepAbs = step;

  newValue = CountReadouts(nTrig, dacReg, threshold);
  if (newValue > thrLevel) {
    do {
      threshold -= step;
      oldValue = newValue;
      newValue = CountReadouts(nTrig, dacReg, threshold);
    } while ((newValue > thrLevel) && (threshold > (stepAbs - 1))
	     && (threshold < (256 - stepAbs)));

    if (oldValue - thrLevel > thrLevel - newValue)
      result = threshold;
    else
      result = threshold + step;
  } else {
    do {
      threshold += step;
      oldValue = newValue;
      newValue = CountReadouts(nTrig, dacReg, threshold);
    } while ((newValue <= thrLevel) && (threshold > (stepAbs - 1))
	     && (threshold < (256 - stepAbs)));

    if (thrLevel - oldValue > newValue - thrLevel)
      result = threshold;
    else
      result = threshold - step;
  }

  if (result > 255)
    result = 255;
  if (result < 0)
    result = 0;

  return result;
}

int32_t CTestboard::fallback_PixelThreshold(int32_t col, int32_t row, int32_t start,
					    int32_t step, int32_t thrLevel, int32_t nTrig, int32_t dacReg,
					    int32_t xtalk, int32_t cals, int32_t trim) {

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  fallback_Daq_Enable(DAQ_READ_SIZE);
  int calRow = row;

  roc_Pix_Trim(col, row, trim);

  if (xtalk) {
    if (row == ROC_NUMROWS - 1)
      calRow = row - 1;
    else
      calRow = row + 1;
  }
  roc_Pix_Cal(col, calRow, cals);
  int32_t res = fallback_Threshold(start, step, thrLevel, nTrig, dacReg);
  roc_ClrCal();

  fallback_Daq_Disable();
  return res;
}

void CTestboard::fallback_ChipThresholdIntern(int32_t start[], int32_t step, int32_t thrLevel, int32_t nTrig, int32_t dacReg, bool xtalk, bool cals, int32_t res[])
{

  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  int32_t thr, startValue;
  for (int col = 0; col < ROC_NUMCOLS; col++)
    {
      roc_Col_Enable(col, 1);
      for (int row = 0; row < ROC_NUMROWS; row++)
	{
	  if (step < 0) startValue = start[col*ROC_NUMROWS + row] + 10;
	  else startValue = start[col*ROC_NUMROWS + row];
	  if (startValue < 0) startValue = 0;
	  else if (startValue > 255) startValue = 255;
	  thr = PixelThreshold(col, row, startValue, step, thrLevel, nTrig, dacReg, xtalk, cals, 15);
	  res[col*ROC_NUMROWS + row] = thr;
	}
      roc_Col_Enable(col, 0);
    }
}

int8_t CTestboard::fallback_ThresholdMap(int32_t nTrig, int32_t dacReg, bool rising, bool xtalk, bool cals, vectorR<int16_t> &thrValue, vectorR<uint32_t> & /*addr*/)
{
  LOG(pxar::logDEBUGRPC) << "(fallback mode) called.";

  thrValue.clear();
  int32_t startValue;
  int32_t step = 1;
  int32_t thrLevel = nTrig/2;
  int32_t roughThr[ROC_NUMROWS * ROC_NUMCOLS], res[ROC_NUMROWS * ROC_NUMCOLS], roughStep;
  if (!rising) {
    startValue = 255;
    roughStep = -4;
  }
  else {
    startValue = 0;
    roughStep = 4;
  }

  for (int i = 0; i < ROC_NUMROWS * ROC_NUMCOLS; i++) { roughThr[i] = startValue; }

  fallback_ChipThresholdIntern(roughThr, roughStep, 0, 1, dacReg, xtalk, cals, roughThr);
  fallback_ChipThresholdIntern(roughThr, step, thrLevel, nTrig, dacReg, xtalk, cals, res);

  for (int i = 0; i < ROC_NUMROWS * ROC_NUMCOLS; i++) { thrValue.push_back(res[i]); }

  return 1;
}
