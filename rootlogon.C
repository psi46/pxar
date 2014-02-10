{
  gStyle->SetOptStat(110111);

  gStyle->SetTickLength( -0.02, "X" ); // tick marks outside
  gStyle->SetTickLength( -0.02, "y");
  gStyle->SetTickLength( -0.01, "z");

  gStyle->SetLabelOffset( 0.022, "x" );
  gStyle->SetLabelOffset( 0.022, "y" );
  gStyle->SetLabelOffset( 0.022, "z" );

  gStyle->SetTitleOffset( 1.3, "x" );
  gStyle->SetTitleOffset( 1.6, "y" );
  gStyle->SetTitleOffset( 1.7, "z" );

  gStyle->SetLabelFont( 62, "X" );
  gStyle->SetLabelFont( 62, "Y" );
  gStyle->SetLabelFont( 62, "z" );

  gStyle->SetTitleFont( 62, "X" );
  gStyle->SetTitleFont( 62, "Y" );
  gStyle->SetTitleFont( 62, "z" );

  gStyle->SetLineWidth(1);// frames
  gStyle->SetHistLineColor(4); // 4=blau
  gStyle->SetHistLineWidth(3);
  gStyle->SetHistFillColor(5); // 5=gelb
  //  gStyle->SetHistFillStyle(4050); // 4050 = half transparent
  gStyle->SetHistFillStyle(1001); // 1001 = solid

  gStyle->SetFrameLineWidth(2);

  gStyle->SetTitleBorderSize(0); // no frame around global title
  gStyle->SetTitleX( 0.36 ); // global title
  gStyle->SetTitleY( 0.98 ); // global title

  gStyle->SetPalette(1); // rainbow colors

  gStyle->SetHistMinimumZero(); // no zero suppression

  gStyle->SetOptDate();

  gROOT->ForceStyle();

  gSystem->Load("/usr/lib64/libf2c.so"); // for Blobel's lvmini
}
